#!/usr/bin/env python3
"""Show both actual SELECT alerts in a staged, normal-damage simulator run."""
from pathlib import Path
import sys,json,subprocess
ROOT=Path(__file__).resolve().parents[1]
from chgame_env import chgame_root
sys.path.insert(0,str(chgame_root()/'tools/chsim'))
from chsim import build
from chdrivelib import Driver,SimTransport
from fbimage import to_image,save_gif
from hosttests import Ctx,compile_test
import gamecfg

def main():
    out=ROOT/'out/switch-alert';out.mkdir(parents=True,exist_ok=True)
    pilot=compile_test(Ctx(ROOT),'pilots',gamecfg.load(ROOT).tests()['pilots'])
    plan=json.loads(subprocess.run([str(pilot),'--capture','6','240','288'],capture_output=True,text=True,check=True).stdout)
    transport=SimTransport(build(ROOT,['BLOOM_CAPTURE'],out=out/'sim.exe'));driver=Driver(transport,'BLOOM');frames=[]
    try:
        driver.handshake();driver.cmd('L1');driver.frames(2);driver.cmd('J 6 0');driver.frames(240)
        for i,row in enumerate(plan):
            keys=row['keys'] if i<24 else (row['keys']&~128)|(128 if i==24 else 0)
            driver.buttons(keys)
            for _ in range(2):
                driver.frames(3);frames.append(to_image(driver.shot(),3))
            state=list(map(int,driver.query('H','BLOOMSTATE').split()[1:]))
            assert state[0]==2 and state[1]==6 and state[3]>0
            assert state[15]==(i<24)
        assert not transport.bugs
    finally:transport.close()
    for name,clip in [('narrow-fire.gif',frames[:48]),('spread-fire.gif',frames[48:])]:
        save_gif(clip,ROOT/'docs'/name,50)
    from PIL import Image
    paired=[]
    for a,b in zip(frames[:48],frames[48:]):
        frame=Image.new('RGB',(768,384));frame.paste(a,(0,0));frame.paste(b,(384,0));paired.append(frame)
    save_gif(paired,out/'fire-mode-alerts.gif',50)
    frames[8].save(out/'narrow.png');frames[56].save(out/'spread.png')
    print('PASS: Narrow fire! and Spread fire!; actual simulator, normal damage, 2.4 seconds each at play speed; zero torn frames')
if __name__=='__main__':main()
