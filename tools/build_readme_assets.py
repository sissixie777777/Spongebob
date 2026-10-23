"""Rebuild the README's original, schematic GIF/SVG artwork using Pillow.

These animations explain behavior; they are not recordings or sensor measurements.
The AI concept render is separate and is never modified by this script.
Usage: python tools/build_readme_assets.py
"""
from pathlib import Path
import math
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets"
OUT.mkdir(exist_ok=True)
S = 2
NAVY = "#0b182a"
PANEL = "#12263c"
LINE = "#28435b"
YELLOW = "#ffda55"
IVORY = "#f4f1df"
MUTED = "#9db5c6"
TEAL = "#6ce4cb"
CORAL = "#ff8c72"


def font(size, bold=False):
    candidates = ([Path("C:/Windows/Fonts/arialbd.ttf")] if bold else
                  [Path("C:/Windows/Fonts/arial.ttf")])
    candidates += [Path("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf" if bold else
                        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")]
    for path in candidates:
        if path.exists():
            return ImageFont.truetype(str(path), round(size * S))
    raise RuntimeError("Install Arial or DejaVu Sans to rebuild the artwork")


class Art:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.im = Image.new("RGB", (w * S, h * S), NAVY)
        self.d = ImageDraw.Draw(self.im)

    def rect(self, box, fill, radius=0, outline=None, width=1):
        self.d.rounded_rectangle(tuple(round(v * S) for v in box),
                                 radius=round(radius * S), fill=fill,
                                 outline=outline, width=round(width * S))

    def ellipse(self, box, fill=None, outline=None, width=1):
        self.d.ellipse(tuple(round(v * S) for v in box), fill=fill,
                       outline=outline, width=round(width * S))

    def line(self, pts, fill, width=1):
        self.d.line([(round(x * S), round(y * S)) for x, y in pts],
                    fill=fill, width=round(width * S))

    def text(self, xy, text, size, fill=IVORY, bold=False):
        self.d.text((round(xy[0] * S), round(xy[1] * S)), text,
                    font=font(size, bold), fill=fill, stroke_width=0)

    def finish(self):
        return self.im.resize((self.w, self.h), Image.Resampling.LANCZOS)


def mascot(a, x, y, scale=1, openness=1, shake=0):
    """Original geometric sponge character. Eye lids visibly cover the eyeballs."""
    def box(v):
        return (x + v[0]*scale + shake, y + v[1]*scale,
                x + v[2]*scale + shake, y + v[3]*scale)
    a.ellipse(box((-14, 252, 240, 278)), "#07111f")
    a.rect(box((188, 38, 244, 235)), "#203b52", 18*scale)
    a.rect(box((205, 64, 238, 192)), "#294960", 11*scale)
    a.rect(box((24, 240, 72, 260)), "#50718a", 9*scale)
    a.rect(box((151, 240, 199, 260)), "#50718a", 9*scale)
    a.rect(box((0, 0, 222, 242)), YELLOW, 29*scale)
    a.rect(box((0, 194, 222, 242)), "#388ac0", 20*scale)
    a.rect(box((0, 194, 222, 216)), "#388ac0")
    a.rect(box((29, 176, 48, 217)), "#388ac0", 5*scale)
    a.rect(box((172, 176, 191, 217)), "#388ac0", 5*scale)
    for bx in (37, 181):
        a.ellipse(box((bx-4, 198, bx+4, 206)), YELLOW)
    for px, py, r in [(20,29,8),(185,24,10),(198,155,7),(20,157,9),
                       (111,22,5),(108,171,4),(195,96,4),(24,108,4)]:
        a.ellipse(box((px-r,py-r,px+r,py+r)), "#eebc39")
    for cx in (70,151):
        a.ellipse(box((cx-35,51,cx+35,127)), IVORY)
        a.ellipse(box((cx-15,72,cx+15,110)), "#78bfda")
        a.ellipse(box((cx-8,77,cx+8,106)), NAVY)
        a.ellipse(box((cx-3,78,cx+3,84)), "#ffffff")
        lid_bottom = 51 + (1-openness)*76
        if lid_bottom > 52:
            a.rect(box((cx-36,49,cx+36,lid_bottom)), YELLOW, 1)
        if openness < 0.12:
            a.line([(x+(cx-22)*scale+shake,y+111*scale),
                    (x+(cx+22)*scale+shake,y+111*scale)], "#b58c2f", 3*scale)
    a.d.arc(tuple(round(v*S) for v in box((86,135,137,159))), 10, 170,
            fill="#a87627", width=max(1,round(3*scale*S)))
    a.rect(box((104,150,114,165)), IVORY, 2*scale)
    a.rect(box((117,150,127,165)), IVORY, 2*scale)


def phase(t):
    # A compressed storyboard, not a real-time 30-minute timer.
    if t < 0.28:
        return 0, 1.0, TEAL, "FRESH AIR.", "Eyes open. All good.", "NORMAL"
    if t < 0.53:
        return 1, 0.48, YELLOW, "GETTING DROWSY.", "Air changes. Eyelids respond.", "TIMING"
    if t < 0.78:
        return 2, 0.06, CORAL, "TIME TO BREATHE.", "30 min high air. A gentle nudge.", "ALERT"
    return 3, 1.0, TEAL, "BRIGHT AGAIN.", "Fresh air resets the reminder.", "RECOVERY"


def save_gif(frames, name, duration):
    # A shared palette prevents palette flicker and keeps files compact.
    samples = [frames[i] for i in (0, len(frames)//3, len(frames)*2//3, len(frames)-1)]
    mosaic = Image.new("RGB", (frames[0].width*2, frames[0].height*2))
    for i, frame in enumerate(samples):
        mosaic.paste(frame, ((i%2)*frame.width, (i//2)*frame.height))
    palette = mosaic.quantize(colors=160, method=Image.Quantize.MEDIANCUT)
    indexed = [im.quantize(palette=palette, dither=Image.Dither.NONE) for im in frames]
    indexed[0].save(OUT/name, save_all=True, append_images=indexed[1:],
                    duration=duration, loop=0, disposal=1, optimize=True)
    frames[0].save(OUT/name.replace(".gif", "-still.png"), optimize=True)


def hero():
    frames = []
    for f in range(96):
        t = f/96
        stage, openness, color, label, sub, state = phase(t)
        a = Art(1000, 430)
        a.rect((1,1,999,429), NAVY, 26, LINE)
        for i in range(9):
            bx = 588 + ((i*61+f*0.6)%375)
            by = 55 + ((i*47-f*0.45)%290)
            r = 3 + i%4*2
            a.ellipse((bx-r,by-r,bx+r,by+r), outline="#203a50")
        a.text((42,32), "A LITTLE SPONGE. A BETTER REMINDER.", 12, MUTED, True)
        a.text((38,65), "DROWSY", 66, IVORY, True)
        a.text((38,137), "SPONGE", 66, YELLOW, True)
        a.ellipse((43,238,51,246), color)
        a.text((64,229), label, 18, color, True)
        a.text((42,265), sub, 17, MUTED)
        a.rect((42,319,470,357), PANEL, 18)
        a.text((60,330), "SENSE    /    EXPRESS    /    REMIND", 12, IVORY, True)
        # Occasional blink in the normal phase; no flashing or fast strobing.
        if stage == 0 and 13 <= f <= 17:
            openness = [0.8,0.35,0.04,0.35,0.8][f-13]
        shake = math.sin(f*math.pi/2)*2 if stage == 2 and (f%12)<6 else 0
        bob = math.sin(f/96*math.tau)*3
        mascot(a,646,69+bob,1.02,openness,shake)
        a.rect((670,362,867,392), PANEL, 15, LINE)
        a.ellipse((684,373,692,381), color)
        a.text((704,370), state, 13, color, True)
        for i in range(4):
            a.rect((42+i*109,390,138+i*109,393), color if i==stage else LINE, 1)
        a.text((809,407), "BEHAVIOR PREVIEW", 9, MUTED)
        frames.append(a.finish())
    save_gif(frames, "hero.gif", 120)


def loop_diagram():
    frames = []
    labels = ["SENSE", "INTERPRET", "EXPRESS", "REMIND"]
    subs = ["MQ135 + smoothing", "Threshold + hysteresis", "Servo + Hall feedback", "30 min + 3 pulses"]
    colors = [TEAL,YELLOW,TEAL,CORAL]
    for f in range(80):
        a = Art(1000,280)
        a.rect((1,1,999,279), NAVY, 22, LINE)
        a.text((32,24), "FROM AIR TO EXPRESSION", 20, IVORY, True)
        a.text((32,56), "One continuous loop. No blocking delays.", 13, MUTED)
        active = f//20
        for i in range(4):
            x = 32+i*246
            a.rect((x,105,x+198,213), PANEL, 15, colors[i] if active==i else LINE, 2 if active==i else 1)
            a.text((x+16,119), f"0{i+1}", 13, colors[i], True)
            a.text((x+16,144), labels[i], 18, IVORY, True)
            a.text((x+16,181), subs[i], 11, MUTED)
            if i < 3:
                a.line([(x+207,158),(x+235,158)], LINE, 2)
                a.line([(x+229,153),(x+235,158),(x+229,163)], LINE, 2)
                if active==i:
                    px = x+207+(f%20)/19*28
                    a.ellipse((px-3,155,px+3,161), colors[i])
        a.text((32,242), "RECOVERY  /  open eyes + cancel vibration + reset timer", 12, TEAL)
        a.text((785,245), "SCHEMATIC ANIMATION", 9, MUTED)
        frames.append(a.finish())
    save_gif(frames,"control-loop.gif",120)


def static_assets():
    # SVG only for static artwork: no JavaScript, SMIL or external resources.
    (OUT/"specs.svg").write_text('''<svg xmlns="http://www.w3.org/2000/svg" width="1000" height="150" viewBox="0 0 1000 150" role="img" aria-label="30 minute trigger, three vibration pulses, three feedback positions, zero blocking delays">
<rect x="1" y="1" width="998" height="148" rx="22" fill="#0b182a" stroke="#28435b"/>
<g stroke="#28435b"><path d="M250 30v90M500 30v90M750 30v90"/></g>
<g font-family="Arial,Helvetica,sans-serif" text-anchor="middle">
<g fill="#ffda55" font-size="38" font-weight="700"><text x="125" y="66">30 MIN</text><text x="375" y="66">3 PULSES</text><text x="625" y="66">3 POSITIONS</text><text x="875" y="66">0 DELAYS</text></g>
<g fill="#9db5c6" font-size="13"><text x="125" y="103">CONTINUOUS HIGH-AIR TRIGGER</text><text x="375" y="103">A TACTILE REMINDER</text><text x="625" y="103">HALL-CONFIRMED / NANO</text><text x="875" y="103">COOPERATIVE STATE MACHINE</text></g></g></svg>''',encoding="utf-8")
    (OUT/"divider.svg").write_text('''<svg xmlns="http://www.w3.org/2000/svg" width="1000" height="44" viewBox="0 0 1000 44"><path d="M0 22H430M570 22H1000" stroke="#9db5c6" stroke-opacity=".3"/><circle cx="477" cy="22" r="4" fill="#6ce4cb"/><circle cx="500" cy="22" r="6" fill="#ffda55"/><circle cx="523" cy="22" r="4" fill="#ff8c72"/></svg>''',encoding="utf-8")
    (OUT/"favicon.svg").write_text('''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64"><rect width="64" height="64" rx="16" fill="#0b182a"/><rect x="9" y="9" width="46" height="46" rx="11" fill="#ffda55"/><path d="M9 44h46v6a5 5 0 0 1-5 5H14a5 5 0 0 1-5-5z" fill="#388ac0"/><g fill="#f4f1df"><ellipse cx="24" cy="29" rx="8" ry="10"/><ellipse cx="41" cy="29" rx="8" ry="10"/></g><g fill="#0b182a"><circle cx="24" cy="30" r="3"/><circle cx="41" cy="30" r="3"/></g><path d="M28 42q5 4 10 0" fill="none" stroke="#a87627" stroke-width="2"/></svg>''',encoding="utf-8")


if __name__ == "__main__":
    hero()
    loop_diagram()
    static_assets()
    for path in sorted(OUT.iterdir()):
        print(f"{path.relative_to(ROOT)}: {path.stat().st_size:,} bytes")
