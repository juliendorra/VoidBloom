#!/usr/bin/env python3
"""Check the rumble timer on a connected Rev0, then restore release firmware."""
from pathlib import Path
import hashlib,json,sys,time,zlib
from chgame_env import chgame_root
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(chgame_root()/'tools'),str(chgame_root()/'tools/chsim')]
import paths  # Registers the configured bootloader host tools.
from chgame_upload.upload import flash_file
from chdrivelib import Driver,SerialTransport,mask_of

def main():
    debug=ROOT/'build/debug/VoidBloom.ino.bin';release=ROOT/'build/release/VoidBloom.ino.bin'
    assert debug.is_file() and release.is_file(),'Build release and debug firmware first'
    transport=None;report={}
    try:
        flash_file(debug,run=True,device='rev0')
        transport=SerialTransport();driver=Driver(transport,'BLOOM')
        driver.handshake();driver.cmd('L1')
        def state():return list(map(int,driver.query('H','BLOOMSTATE').split()[1:]))
        def press(button):driver.buttons(mask_of(button));driver.frames(1);driver.buttons(0)
        boot=state();assert boot[0]==0 and boot[20:22]==[0,0],boot
        press('A');driver.frames(70)
        def blast():
            driver.buttons(mask_of('B'));driver.frames(1)
            sample=state();driver.buttons(0)
            assert sample[20]==235 and 0<sample[21]<=35,sample
            return sample
        bomb=blast()
        press('START');paused=state();assert paused[0]==3 and paused[20:22]==[0,0],paused
        driver.frames(1);press('START');driver.frames(76)
        active=blast();ripples=[{'flash':30,'strength':active[20]}]
        for frames,flash,strength in [(6,24,0),(4,20,210),(6,14,0),(4,10,185),(10,0,0)]:
            driver.frames(frames);sample=state()
            assert sample[20]==strength,(flash,strength,sample)
            ripples.append({'flash':flash,'strength':sample[20]})
        driver.frames(155);active=blast()
        time.sleep(.1) # No game frames: the 35 ms refresh must expire by IRQ.
        expired=state();assert expired[5]==active[5] and expired[20:22]==[0,0],expired
        assert not transport.bugs
        report={'board':'CHGame Rev0 CX35','driver':'Pinball GPIO1/PC0 TIM2 CH4',
                'boot_off':True,'bomb_strength':bomb[20],'bomb_refresh_remaining_ms':bomb[21],
                'bomb_ripple_frames':30,'bomb_ripple_samples':ripples,
                'pause_immediate_stop':True,'expired_without_game_frames':True,
                'debug_firmware_sha256':hashlib.sha256(debug.read_bytes()).hexdigest(),
                'limits':['Timer/controller state verified on the actual device; motor feel is a human check.']}
    finally:
        if transport:transport.close()
        restored=flash_file(release,run=True,device='rev0')
        assert restored['crc32']==zlib.crc32(release.read_bytes())
        if report:
            report['release_restored']=True;report['release_crc32']=f"{restored['crc32']:08X}"
            out=ROOT/'out/rumble';out.mkdir(parents=True,exist_ok=True)
            (out/'device-validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS: physical rumble boot-off, three visual-synced bomb ripples, pause stop, IRQ expiry without game frames; release restored')
if __name__=='__main__':main()
