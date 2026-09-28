import Foundation
import CoreMIDI
import Darwin

private let ccDown: UInt8 = 46
private let ccUp: UInt8 = 47
private let ccMute: UInt8 = 43  // fader button 7 (under monitor fader); was 48
private let ccSubMute: UInt8 = 44  // fader button 8 (under sub / Cue 2 fader)
private let ccSub: UInt8 = 28
private let ccMon: UInt8 = 27
private let ccScrubBar: UInt8 = 29
private let ccScrubBeat: UInt8 = 30
private let ccScrubTick: UInt8 = 31
private let scrubCCs: Set<UInt8> = [29, 30, 31]
private let rubatoCCs: Set<UInt8> = [21, 22, 23, 24, 25, 26, 76, 77]
private let step: Double = 0.0125
private let mixerPort: UInt16 = 4710

private func log(_ s: String) {
    let ts = ISO8601DateFormatter().string(from: Date())
    FileHandle.standardError.write(Data("[\(ts)] \(s)\n".utf8))
}

private final class Mixer {
    private var fd: Int32 = -1
    private var funcId = 1000
    private var monitorOut: String?
    private var cached: Double = 0.1875
    private var cachedMute: Bool = false
    private let q = DispatchQueue(label: "ua.mixer")
    private var rx = Data()
    private struct CueSend {
        var path: String
        var name: String
        var mix: Double
        var lastWritten: Double
    }
    private var cueSends: [CueSend] = []
    private var pendingSub: UInt8?
    private var subBusy = false
    private var master: Double = 1.0
    private var haveMaster = false
    private var lastCueDiscover = Date.distantPast
    private var pendingMon: UInt8?
    private var monBusy = false
    private var lastMonWritten: Double = -1
    private var monMix: Double = 0.1875
    private var monMaster: Double = 1.0
    private var haveMonMaster = false
    private var cueMuted = false
    private var cueMuteSaved: [Double] = []

    func start() {
        q.async { self.connectAndDiscover() }
    }

    func nudge(_ delta: Double) {
        q.async { self.nudgeLocked(delta) }
    }

    func toggleMute() {
        q.async { self.toggleMuteLocked() }
    }

    func toggleCueMute() {
        q.async { self.toggleCueMuteLocked() }
    }

    func setSub(_ cc: UInt8) {
        q.async {
            self.pendingSub = cc
            if !self.subBusy {
                self.subBusy = true
                while let v = self.pendingSub {
                    self.pendingSub = nil
                    self.setSubLocked(v)
                }
                self.subBusy = false
            }
        }
    }

    func setMon(_ cc: UInt8) {
        q.async {
            self.pendingMon = cc
            if !self.monBusy {
                self.monBusy = true
                while let v = self.pendingMon {
                    self.pendingMon = nil
                    self.setMonLocked(v)
                }
                self.monBusy = false
            }
        }
    }

    private func connectAndDiscover() {
        closeFd()
        fd = socket(AF_INET, SOCK_STREAM, 0)
        guard fd >= 0 else {
            log("mixer socket failed")
            return
        }
        var addr = sockaddr_in()
        addr.sin_len = UInt8(MemoryLayout<sockaddr_in>.size)
        addr.sin_family = sa_family_t(AF_INET)
        addr.sin_port = mixerPort.bigEndian
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr)
        let rc = withUnsafePointer(to: &addr) {
            $0.withMemoryRebound(to: sockaddr.self, capacity: 1) {
                Darwin.connect(fd, $0, socklen_t(MemoryLayout<sockaddr_in>.size))
            }
        }
        if rc != 0 {
            log("mixer connect failed errno=\(errno)")
            closeFd()
            return
        }
        var tv = timeval(tv_sec: 2, tv_usec: 0)
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, socklen_t(MemoryLayout<timeval>.size))
        log("mixer connected")
        discoverMonitor()
        discoverCue2()
    }

    private func closeFd() {
        if fd >= 0 { Darwin.close(fd) }
        fd = -1
        rx.removeAll()
    }

    private func sendCmd(_ cmd: String) -> [String: Any]? {
        if fd < 0 { connectAndDiscover() }
        guard fd >= 0 else { return nil }
        var data = Array(cmd.utf8)
        data.append(0)
        let n = data.withUnsafeBufferPointer { buf in
            Darwin.send(fd, buf.baseAddress, buf.count, 0)
        }
        if n < 0 {
            log("mixer send failed")
            closeFd()
            return nil
        }
        return recvJSON()
    }

    private func recvJSON() -> [String: Any]? {
        var tmp = [UInt8](repeating: 0, count: 8192)
        let deadline = Date().addingTimeInterval(2)
        while Date() < deadline {
            if let nul = rx.firstIndex(of: 0) {
                let part = rx.prefix(upTo: nul)
                rx.removeSubrange(...nul)
                if part.isEmpty { continue }
                return (try? JSONSerialization.jsonObject(with: part)) as? [String: Any]
            }
            let n = Darwin.recv(fd, &tmp, tmp.count, 0)
            if n > 0 {
                rx.append(contentsOf: tmp.prefix(n))
                continue
            }
            if n == 0 {
                log("mixer closed")
                closeFd()
                return nil
            }
            if errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR {
                continue
            }
            log("mixer recv errno=\(errno)")
            closeFd()
            return nil
        }
        return nil
    }

    private func childIds(_ path: String) -> [String] {
        guard let msg = sendCmd("get \(path)") else { return [] }
        let data = msg["data"] as? [String: Any]
        let ch = data?["children"] as? [String: Any] ?? [:]
        return ch.keys.sorted { (Int($0) ?? 0) < (Int($1) ?? 0) }
    }

    private func props(_ msg: [String: Any]?) -> [String: Any] {
        ((msg?["data"] as? [String: Any])?["properties"] as? [String: Any]) ?? [:]
    }

    private func discoverMonitor() {
        guard let devices = sendCmd("get /devices") else { return }
        let data = devices["data"] as? [String: Any]
        let children = (data?["children"] as? [String: Any]) ?? [:]
        for d in children.keys.sorted() {
            for o in childIds("/devices/\(d)/outputs") {
                let msg = sendCmd("get /devices/\(d)/outputs/\(o)")
                let p = props(msg)
                let io = (p["IOType"] as? [String: Any])?["value"] as? String
                if io == "Monitor", p["CRMonitorLevelTapered"] != nil {
                    monitorOut = "/devices/\(d)/outputs/\(o)"
                    if let t = (p["CRMonitorLevelTapered"] as? [String: Any])?["value"] as? Double {
                        cached = t
                    }
                    if let m = (p["Mute"] as? [String: Any])?["value"] as? Bool {
                        cachedMute = m
                    }
                    log("monitor \(monitorOut!) tapered=\(cached) mute=\(cachedMute)")
                    return
                }
            }
        }
        log("no monitor output found")
    }

    private func discoverCue2() {
        var found: [CueSend] = []
        guard let devices = sendCmd("get /devices") else { return }
        let data = devices["data"] as? [String: Any]
        let children = (data?["children"] as? [String: Any]) ?? [:]
        for d in children.keys.sorted() {
            scanSends(prefix: "/devices/\(d)/inputs", into: &found)
            scanSends(prefix: "/devices/\(d)/auxs", into: &found)
        }
        lastCueDiscover = Date()
        if found.isEmpty {
            if cueSends.isEmpty {
                log("cue2 VCA: no active Cue 2 sends")
            } else {
                log("cue2 rediscover empty, keeping last mix")
            }
        } else {
            cueSends = found
            let names = cueSends.map { "\($0.name)=\(String(format: "%.3f", $0.mix))" }.joined(separator: ", ")
            log("cue2 mix: \(names)")
        }
    }

    private func scanSends(prefix: String, into found: inout [CueSend]) {
        for strip in childIds(prefix) {
            let stripPath = "\(prefix)/\(strip)"
            let stripMsg = sendCmd("get \(stripPath)")
            let stripName = (props(stripMsg)["Name"] as? [String: Any])?["value"] as? String ?? strip
            for sid in childIds("\(stripPath)/sends") {
                let sp = "\(stripPath)/sends/\(sid)"
                let p = props(sendCmd("get \(sp)"))
                let id = (p["ID"] as? [String: Any])?["value"] as? String
                let sendName = (p["Name"] as? [String: Any])?["value"] as? String
                guard id == "cue2" || sendName == "CUE 2" else { continue }
                let tapered = (p["GainTapered"] as? [String: Any])?["value"] as? Double ?? 0
                guard tapered > 0.0001 else { continue }
                if let old = cueSends.first(where: { $0.path == sp }) {
                    found.append(old)
                } else {
                    found.append(CueSend(path: sp, name: stripName, mix: tapered, lastWritten: tapered))
                }
            }
        }
    }

    private func currentTapered(_ path: String, fallback: Double) -> Double {
        let p = props(sendCmd("get \(path)"))
        return (p["GainTapered"] as? [String: Any])?["value"] as? Double ?? fallback
    }

    private func writeSend(_ i: Int, _ value: Double) {
        let v = min(1.0, max(0.0, value))
        funcId += 1
        _ = sendCmd("set \(cueSends[i].path)/GainTapered/value?context_type=main&func_id=\(funcId) \(v)")
        cueSends[i].lastWritten = v
    }

    private func setSubLocked(_ cc: UInt8) {
        if fd < 0 { connectAndDiscover() }
        if cueMuted {
            // fader move cancels mute pickup
            cueMuted = false
            cueMuteSaved = []
            haveMaster = false
            log("cue2 mute cleared by fader")
        }
        let needDiscover = cueSends.isEmpty
            ? Date().timeIntervalSince(lastCueDiscover) > 2
            : (cc >= 126 && Date().timeIntervalSince(lastCueDiscover) > 3)
        if needDiscover { discoverCue2() }
        let newMaster = Double(cc) / 127.0

        if !haveMaster {
            haveMaster = true
            if newMaster >= 0.02 {
                for i in cueSends.indices {
                    let current = currentTapered(cueSends[i].path, fallback: cueSends[i].mix)
                    cueSends[i].mix = min(1.0, max(0.0, current / newMaster))
                    cueSends[i].lastWritten = current
                }
                master = newMaster
                log(String(format: "cue2 VCA pickup master=%.2f (no jump)", master))
                return
            }
            master = 0
            for i in cueSends.indices { writeSend(i, 0) }
            log("cue2 VCA master=0")
            return
        }

        if master >= 0.02 {
            for i in cueSends.indices {
                let current = currentTapered(cueSends[i].path, fallback: cueSends[i].lastWritten)
                if abs(current - cueSends[i].lastWritten) > 0.012 {
                    cueSends[i].mix = min(1.0, max(0.0, current / master))
                    log("cue2 mix \(cueSends[i].name)=\(String(format: "%.3f", cueSends[i].mix))")
                }
            }
        }

        if abs(newMaster - master) < 0.004 { return }
        master = newMaster
        for i in cueSends.indices {
            writeSend(i, cueSends[i].mix * master)
        }
    }

    private func nudgeLocked(_ delta: Double) {
        if fd < 0 || monitorOut == nil { connectAndDiscover() }
        guard let out = monitorOut else {
            log("nudge dropped, no monitor")
            return
        }
        if let msg = sendCmd("get \(out)") {
            let p = props(msg)
            if let t = (p["CRMonitorLevelTapered"] as? [String: Any])?["value"] as? Double {
                cached = t
            }
        }
        let next = min(1.0, max(0.0, cached + delta))
        funcId += 1
        let cmd = "set \(out)/CRMonitorLevelTapered/value?context_type=main&func_id=\(funcId) \(next)"
        if let reply = sendCmd(cmd) {
            if let v = reply["data"] as? Double {
                cached = v
            } else {
                cached = next
            }
            log(String(format: "monitor %.4f", cached))
            lastMonWritten = cached
            if haveMonMaster && monMaster >= 0.02 {
                monMix = cached / monMaster
            }
        } else {
            log("nudge set failed")
        }
    }

    private func writeMon(_ out: String, _ value: Double) {
        let v = min(1.0, max(0.0, value))
        funcId += 1
        let cmd = "set \(out)/CRMonitorLevelTapered/value?context_type=main&func_id=\(funcId) \(v)"
        if let reply = sendCmd(cmd) {
            if let t = reply["data"] as? Double {
                cached = t
            } else {
                cached = v
            }
            lastMonWritten = cached
        }
    }

    private func setMonLocked(_ cc: UInt8) {
        if fd < 0 || monitorOut == nil { connectAndDiscover() }
        guard let out = monitorOut else { return }
        if let msg = sendCmd("get \(out)") {
            let p = props(msg)
            if let t = (p["CRMonitorLevelTapered"] as? [String: Any])?["value"] as? Double {
                cached = t
            }
        }
        let newMaster = Double(cc) / 127.0

        if !haveMonMaster {
            haveMonMaster = true
            if newMaster >= 0.02 {
                monMix = max(0.0, cached / newMaster)
                lastMonWritten = cached
                monMaster = newMaster
                log(String(format: "monitor VCA pickup master=%.2f mix=%.3f (no jump)", monMaster, monMix))
                return
            }
            monMix = cached
            monMaster = 0
            writeMon(out, 0)
            log("monitor VCA master=0")
            return
        }

        if monMaster >= 0.02, abs(cached - lastMonWritten) > 0.012 {
            monMix = max(0.0, cached / monMaster)
            log(String(format: "monitor mix=%.3f", monMix))
        }

        if abs(newMaster - monMaster) < 0.004 { return }
        monMaster = newMaster
        writeMon(out, monMix * monMaster)
    }

    private func toggleMuteLocked() {
        if fd < 0 || monitorOut == nil { connectAndDiscover() }
        guard let out = monitorOut else {
            log("mute dropped, no monitor")
            return
        }
        if let msg = sendCmd("get \(out)") {
            let p = props(msg)
            if let m = (p["Mute"] as? [String: Any])?["value"] as? Bool {
                cachedMute = m
            }
        }
        let next = !cachedMute
        funcId += 1
        let lit = next ? "true" : "false"
        let cmd = "set \(out)/Mute/value?context_type=main&func_id=\(funcId) \(lit)"
        if let reply = sendCmd(cmd) {
            if let v = reply["data"] as? Bool {
                cachedMute = v
            } else {
                cachedMute = next
            }
            log("mute \(cachedMute)")
        } else {
            log("mute set failed")
        }
    }

    private func toggleCueMuteLocked() {
        if fd < 0 { connectAndDiscover() }
        if cueSends.isEmpty { discoverCue2() }
        if cueSends.isEmpty {
            log("cue2 mute dropped, no sends")
            return
        }
        if !cueMuted {
            cueMuteSaved = cueSends.map { currentTapered($0.path, fallback: $0.lastWritten) }
            for i in cueSends.indices { writeSend(i, 0) }
            cueMuted = true
            log("cue2 mute on")
        } else {
            let saved = cueMuteSaved
            for i in cueSends.indices {
                let v = i < saved.count ? saved[i] : cueSends[i].mix * master
                writeSend(i, v)
            }
            cueMuteSaved = []
            cueMuted = false
            haveMaster = false
            log("cue2 mute off")
        }
    }

}

private let mixer = Mixer()

private var client = MIDIClientRef()
private var port = MIDIPortRef()
private var connected = Set<MIDIEndpointRef>()
private var lastFire: [UInt8: Date] = [:]
private var lastSubLog: UInt8 = 255
private var lastMonLog: UInt8 = 255

// Absolute CC 29–31 → Key Command pulses on virtual "LCXL Scrub KC".
// Logic Playhead CA Min/Max is broken; Learn these as Forward/Rewind by Bar|Beat|Division.
// Pulses: 29→CC90/91  30→CC92/93  31→CC94/95  (127 then 0 on ch1)
private var scrubSource = MIDIEndpointRef()
private var lastScrubAbs: [UInt8: UInt8] = [:]
private var lastScrubDir: [UInt8: Int] = [:]  // +1 / -1
private var lastScrubRail: [UInt8: Int] = [:]  // 0 none, +1 high(127), -1 low(0)

private let scrubKCFwd: [UInt8: UInt8] = [29: 90, 30: 92, 31: 94]
private let scrubKCBack: [UInt8: UInt8] = [29: 91, 30: 93, 31: 95]
/// Extra KC fires per encoder step (Bar×4 ≈ 1 bar when Division is /4 in 4/4).
private let scrubKCMultiply: [UInt8: Int] = [29: 4, 30: 1, 31: 1]
private let scrubKCMaxPulses = 64  // only clamp insane bursts; don't punish fast twists

private func ensureScrubSource() {
    if scrubSource != 0 { return }
    var src = MIDIEndpointRef()
    let err = MIDISourceCreate(client, "LCXL Scrub KC" as CFString, &src)
    if err != noErr {
        log("scrub KC virtual source failed err=\(err)")
        return
    }
    scrubSource = src
    log("scrub KC out: LCXL Scrub KC (90/91 bar, 92/93 beat, 94/95 division)")
}

private func midiSend(_ bytes: [UInt8]) {
    guard scrubSource != 0 else { return }
    var buffer = [UInt8](repeating: 0, count: 256)
    buffer.withUnsafeMutableBytes { raw in
        let list = raw.bindMemory(to: MIDIPacketList.self).baseAddress!
        let pkt = MIDIPacketListInit(list)
        _ = bytes.withUnsafeBufferPointer { bp in
            MIDIPacketListAdd(list, 256, pkt, 0, bytes.count, bp.baseAddress!)
        }
        let e = MIDIReceived(scrubSource, list)
        if e != noErr { log("scrub KC MIDIReceived err=\(e)") }
    }
}

private func pulseKC(_ cc: UInt8) {
    midiSend([0xB0, cc, 127])
    midiSend([0xB0, cc, 0])
}

private func emitScrubRel(cc: UInt8, absVal: UInt8) {
    ensureScrubSource()
    guard scrubSource != 0 else { return }
    guard let fwd = scrubKCFwd[cc], let back = scrubKCBack[cc] else { return }
    let mult = scrubKCMultiply[cc] ?? 1

    func fire(_ dir: Int, steps: Int) {
        guard steps > 0 else { return }
        let n = min(steps, scrubKCMaxPulses)
        let kc = dir > 0 ? fwd : back
        lastScrubDir[cc] = dir
        for _ in 0..<n { pulseKC(kc) }
        log("scrub KC CC\(cc) dir=\(dir) → \(n)×CC\(kc)")
    }

    // After hitting a rail, keep pulsing while XL re-sends 0/127; only leave rail
    // once the knob backs off so reverse isn't inverted.
    let rail = lastScrubRail[cc] ?? 0
    if rail == 1 {
        if absVal >= 127 {
            fire(1, steps: mult)
            lastScrubAbs[cc] = 127
            return
        }
        if absVal <= 120 {
            lastScrubRail[cc] = 0
            lastScrubAbs[cc] = absVal
            log("scrub KC CC\(cc) left high rail @\(absVal)")
            return
        }
        lastScrubAbs[cc] = absVal
        return
    }
    if rail == -1 {
        if absVal == 0 {
            fire(-1, steps: mult)
            lastScrubAbs[cc] = 0
            return
        }
        if absVal >= 7 {
            lastScrubRail[cc] = 0
            lastScrubAbs[cc] = absVal
            log("scrub KC CC\(cc) left low rail @\(absVal)")
            return
        }
        lastScrubAbs[cc] = absVal
        return
    }

    guard let prev = lastScrubAbs[cc] else {
        lastScrubAbs[cc] = absVal
        log("scrub KC CC\(cc) abs=\(absVal) (baseline)")
        return
    }

    let delta = Int(absVal) - Int(prev)
    lastScrubAbs[cc] = absVal
    if delta == 0 { return }

    let dir = delta > 0 ? 1 : -1
    fire(dir, steps: abs(delta) * mult)

    if absVal >= 127 && dir > 0 {
        lastScrubRail[cc] = 1
        log("scrub KC CC\(cc) high rail")
    } else if absVal == 0 && dir < 0 {
        lastScrubRail[cc] = -1
        log("scrub KC CC\(cc) low rail")
    }
}

private func endpointName(_ ep: MIDIEndpointRef) -> String {
    var cf: Unmanaged<CFString>?
    let err = MIDIObjectGetStringProperty(ep, kMIDIPropertyDisplayName, &cf)
    if err == noErr, let s = cf?.takeRetainedValue() as String? { return s }
    return "(unnamed)"
}


// Forward Rubato CCs 21–26/76/77 absolute onto virtual "LCXL Rubato" so Control Surface can't swallow them.
private var rubatoSource = MIDIEndpointRef()

private func ensureRubatoSource() {
    if rubatoSource != 0 { return }
    var src = MIDIEndpointRef()
    let err = MIDISourceCreate(client, "LCXL Rubato" as CFString, &src)
    if err != noErr {
        log("rubato virtual source failed err=\(err)")
        return
    }
    rubatoSource = src
    log("rubato forward out: LCXL Rubato")
}

private var lastRubatoLog: UInt8 = 255
private var lastRubatoCC: UInt8 = 255

private func emitRubatoFwd(cc: UInt8, absVal: UInt8) {
    ensureRubatoSource()
    guard rubatoSource != 0 else { return }
    if lastRubatoCC != cc || lastRubatoLog == 255 || abs(Int(absVal) - Int(lastRubatoLog)) >= 8 {
        log("rubato CC \(cc) \(absVal) → LCXL Rubato")
        lastRubatoLog = absVal
        lastRubatoCC = cc
    }
    var buffer = [UInt8](repeating: 0, count: 256)
    buffer.withUnsafeMutableBytes { raw in
        let list = raw.bindMemory(to: MIDIPacketList.self).baseAddress!
        let pkt = MIDIPacketListInit(list)
        let data: [UInt8] = [0xB0, cc, absVal]
        _ = data.withUnsafeBufferPointer { bp in
            MIDIPacketListAdd(list, 256, pkt, 0, 3, bp.baseAddress!)
        }
        let e = MIDIReceived(rubatoSource, list)
        if e != noErr { log("rubato MIDIReceived err=\(e)") }
    }
}

private func isXL(_ name: String) -> Bool {
    let n = name.lowercased()
    if n.contains("scrub rel") || n.contains("scrub kc") || n.contains("rubato") { return false } // our virtual outs — never echo
    return n.contains("lcxl") || n.contains("launch control xl")
}

private func fire(_ cc: UInt8, val: UInt8, src: String) {
    if cc == ccSub {
        mixer.setSub(val)
        if lastSubLog == 255 || abs(Int(val) - Int(lastSubLog)) >= 8 {
            log("CC 28 sub \(val) from \(src)")
            lastSubLog = val
        }
        return
    }
    if cc == ccMon {
        mixer.setMon(val)
        if lastMonLog == 255 || abs(Int(val) - Int(lastMonLog)) >= 8 {
            log("CC 27 mon \(val) from \(src)")
            lastMonLog = val
        }
        return
    }
    if scrubCCs.contains(cc) {
        emitScrubRel(cc: cc, absVal: val)
        return
    }
    if rubatoCCs.contains(cc) {
        emitRubatoFwd(cc: cc, absVal: val)
        return
    }
    // Button mutes: 43 = monitor, 44 = Cue 2 / sub. 46/47/48 free for Logic.
    guard cc == ccMute || cc == ccSubMute else { return }
    log("CC \(cc) val=\(val) from \(src)")
    guard val > 0 else { return }
    let now = Date()
    if let prev = lastFire[cc], now.timeIntervalSince(prev) < 0.08 { return }
    lastFire[cc] = now
    if cc == ccMute { mixer.toggleMute() }
    else { mixer.toggleCueMute() }
}

private func parseMIDI(_ bytes: UnsafeBufferPointer<UInt8>, src: String) {
    var i = 0
    var running: UInt8 = 0
    while i < bytes.count {
        var status = bytes[i]
        if status < 0x80 {
            if running == 0 { i += 1; continue }
            status = running
        } else {
            i += 1
            if status < 0xF8 { running = status }
        }
        if status == 0xF0 {
            while i < bytes.count && bytes[i] != 0xF7 { i += 1 }
            if i < bytes.count { i += 1 }
            running = 0
            continue
        }
        let cmd = status & 0xF0
        if cmd == 0xB0 || cmd == 0x90 || cmd == 0x80 {
            guard i + 1 < bytes.count else { return }
            let d1 = bytes[i]
            let d2 = bytes[i + 1]
            i += 2
            if d1 == ccMute || d1 == ccSubMute || d1 == ccSub || d1 == ccMon || scrubCCs.contains(d1) || rubatoCCs.contains(d1) {
                let val = (cmd == 0x80) ? 0 : d2
                fire(d1, val: val, src: src)
            }
        } else if cmd == 0xC0 || cmd == 0xD0 {
            if i < bytes.count { i += 1 }
        } else if status >= 0xF8 {
            continue
        } else if status == 0xF1 || status == 0xF3 {
            if i < bytes.count { i += 1 }
        } else if status == 0xF2 {
            i += min(2, bytes.count - i)
        } else {
            i += min(2, bytes.count - i)
        }
    }
}

private func handlePacketList(_ pktList: UnsafePointer<MIDIPacketList>, src: String) {
    let rawList = UnsafeRawPointer(pktList)
    guard let packetOffset = MemoryLayout<MIDIPacketList>.offset(of: \MIDIPacketList.packet) else { return }
    var p = rawList.advanced(by: packetOffset)
    for _ in 0..<Int(pktList.pointee.numPackets) {
        let packet = p.assumingMemoryBound(to: MIDIPacket.self)
        let length = Int(packet.pointee.length)
        withUnsafePointer(to: packet.pointee.data) { dataPtr in
            let bytes = UnsafeRawPointer(dataPtr).assumingMemoryBound(to: UInt8.self)
            parseMIDI(UnsafeBufferPointer(start: bytes, count: length), src: src)
        }
        p = UnsafeRawPointer(MIDIPacketNext(packet))
    }
}

private let readProc2: MIDIReadProc = { pktList, _, srcConn in
    guard let srcConn else {
        handlePacketList(pktList, src: "XL")
        return
    }
    let ep = MIDIEndpointRef(UInt32(UInt(bitPattern: srcConn)))
    handlePacketList(pktList, src: endpointName(ep))
}

private func connectSources() {
    let n = MIDIGetNumberOfSources()
    var seen = Set<MIDIEndpointRef>()
    for i in 0..<n {
        let src = MIDIGetSource(i)
        seen.insert(src)
        let name = endpointName(src)
        guard isXL(name) else { continue }
        if connected.contains(src) { continue }
        let ref = UnsafeMutableRawPointer(bitPattern: UInt(src))
        let err = MIDIPortConnectSource(port, src, ref)
        if err == noErr {
            connected.insert(src)
            log("listening on \(name)")
        } else {
            log("connect failed \(name) err=\(err)")
        }
    }
    connected = connected.intersection(seen)
}

private let notifyProc: MIDINotifyProc = { message, _ in
    if message.pointee.messageID == .msgSetupChanged {
        connectSources()
    }
}

private func listSources() {
    let n = MIDIGetNumberOfSources()
    if n == 0 { print("No MIDI sources."); return }
    for i in 0..<n {
        let src = MIDIGetSource(i)
        let name = endpointName(src)
        let mark = isXL(name) ? "  [XL]" : ""
        print("\(i): \(name)\(mark)")
    }
}

let args = Set(CommandLine.arguments.dropFirst())
if args.contains("--list") {
    listSources()
    exit(0)
}
if args.contains("--down") || args.contains("--up") || args.contains("--mute") {
    mixer.start()
    Thread.sleep(forTimeInterval: 0.4)
    if args.contains("--mute") {
        mixer.toggleMute()
    } else {
        mixer.nudge(args.contains("--down") ? -step : step)
    }
    Thread.sleep(forTimeInterval: 0.4)
    exit(0)
}

mixer.start()

var err = MIDIClientCreate("LCXL Monitor Hotkeys" as CFString, notifyProc, nil, &client)
if err != noErr { fputs("MIDIClientCreate \(err)\n", stderr); exit(1) }
err = MIDIInputPortCreate(client, "input" as CFString, readProc2, nil, &port)
if err != noErr { fputs("MIDIInputPortCreate \(err)\n", stderr); exit(1) }
connectSources()
if connected.isEmpty { log("no XL source yet; waiting") }
ensureScrubSource()
ensureRubatoSource()
log("CC 27/28/43/44 console; scrub→LCXL Scrub KC (KC pulses); rubato 21–26/76/77→LCXL Rubato")
CFRunLoopRun()
