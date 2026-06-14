#!/usr/bin/env python3
"""The Bad King - automated player v4."""
import requests, time, re, sys

BASE = "http://localhost:5000"
last = ""

def send(c):
    global last
    r = requests.post(f"{BASE}/command", json={"char": c})
    last = r.json()["output"]
    return last

def get_loc(text=None):
    """Extract location name from game output."""
    if text is None: text = last
    for line in text.replace("\r", "").split("\n"):
        line = line.strip()
        m = re.match(r'^(.+?)\s{2,}HP \d+,MP \d+$', line)
        if m:
            return m.group(1).strip()
    return ""

def is_battle(text=None):
    if text is None: text = last
    return ("Your HP:" in text and "Your MP:" in text) or \
           "commands used in battle" in text

def is_menu(text=None):
    if text is None: text = last
    return "Where?" in text

def is_continue(text=None):
    """A 'Press a/any key to continue' prompt -- any key (incl. space) advances."""
    if text is None: text = last
    t = text.lower()
    return "press a key" in t or "press any key" in t

def info(text=None):
    if text is None: text = last
    t = text.replace("\r", "")
    b = re.search(r'HP (\d+)/(\d+)', t)            # battle/status: HP cur/max
    tr = re.search(r'HP (-?\d+),MP (-?\d+)', t)    # travel: HP cur, MP cur
    lv = re.search(r'Level (\d+)', t)
    gd = re.search(r'(\d+) Gold', t)
    if b:
        hp, maxhp = int(b.group(1)), int(b.group(2))
    elif tr:
        hp, maxhp = int(tr.group(1)), 0            # max not shown in travel mode
    else:
        hp, maxhp = 0, 0
    return {"loc": get_loc(t), "hp": hp, "maxhp": maxhp,
            "level": int(lv.group(1)) if lv else 0,
            "gold": int(gd.group(1)) if gd else 0}

def pinfo():
    s = info()
    print(f"  [{s['loc']}] Lv{s['level']} HP {s['hp']}/{s['maxhp']} G{s['gold']}")
    return s

def fight():
    for _ in range(300):
        out = send("f")
        low = out.lower()
        if "defeated the" in low:                  # "You have defeated the %s."
            return out
        if "too much for you" in low:              # "Poor %s ... too much for you."
            send(" ")                              # dismiss "Press any key to continue"
            return out
        loc = get_loc(out)
        if loc and "Command?" in out and not is_battle(out):
            return out
        time.sleep(0.02)
    return "TIMEOUT"

def stabilize():
    """Return current location, resolving only clear states. Never flail --
    sending blind commands when the state is unknown corrupts the game."""
    refreshed = False
    for _ in range(6):
        if is_continue():
            send(" ")                             # any key advances a continue prompt
            continue
        if is_battle():
            fight()
            continue
        if is_menu():
            send("x")                             # cancel the "Where?" move menu
            continue
        loc = get_loc()
        if loc:
            return loc
        # One safe refresh: 'l' (look) reprints the location at a travel
        # prompt. Don't repeat it -- blind commands corrupt game state.
        if "Command?" in last and not refreshed:
            send("l")
            refreshed = True
            continue
        break
    return get_loc()

def move_to(dest_letter):
    """Send m then dest_letter, handling encounters."""
    out = send("m")
    if "Where?" not in out:
        if is_battle(out): fight()
        out = send("m")
    if "Where?" in out:
        out = send(dest_letter)
        if is_battle(out):
            fight()
    return get_loc()

def find_path(start, target, pm):
    """BFS over the path_map graph. Returns a list of move-letters from
    start to target, or None if unreachable."""
    from collections import deque
    if start == target:
        return []
    if start not in pm:
        return None
    prev = {start: None}
    q = deque([start])
    while q:
        cur = q.popleft()
        for nbr, letter in pm.get(cur, {}).items():
            if nbr not in prev:
                prev[nbr] = (cur, letter)
                if nbr == target:
                    path = []
                    n = nbr
                    while prev[n] is not None:
                        path.append(prev[n][1])
                        n = prev[n][0]
                    return list(reversed(path))
                q.append(nbr)
    return None

def nav(target, pm):
    """Navigate to target location using BFS over the path_map."""
    for attempt in range(80):
        loc = stabilize()
        if loc == target:
            return True
        path = find_path(loc, target, pm)
        if not path:
            if loc:
                print(f"  No route: {loc} -> {target}")
            return False
        move_to(path[0])                          # advance one hop; re-evaluate next loop
    return stabilize() == target

def grind(n):
    """Do n search/fight cycles."""
    for i in range(n):
        out = send("s")
        if is_battle(out) or "attacked by" in out.lower():
            result = fight()
            if "too much for you" in result.lower():
                print("  DIED!"); return False
        if i % 15 == 14:
            pinfo()
    return True

# Path map
pm = {
    "Tree Square": {"Squirrel's Nook Inn": "a", "Grubby's Armory": "b",
                    "Mura's Magic Shop": "c", "Forest Gate": "d"},
    "Squirrel's Nook Inn": {"Tree Square": "a"},
    "Grubby's Armory": {"Tree Square": "a"},
    "Mura's Magic Shop": {"Tree Square": "a"},
    "Forest Gate": {"Tree Square": "a", "Forest Path": "b", "Lake Shore": "c"},
    "Forest Path": {"Forest Gate": "a", "Tall Tree": "b", "Bear Rock": "c"},
    "Tall Tree": {"Forest Path": "a"},
    "Bear Rock": {"Forest Path": "a", "Shadowy Path": "b"},
    "Shadowy Path": {"Bear Rock": "a", "Big Oaks": "b"},
    "Big Oaks": {"Shadowy Path": "a", "Meadow": "b"},
    "Meadow": {"Big Oaks": "a", "Babbling Brook": "b"},
    "Babbling Brook": {"Meadow": "a", "Rushing Falls": "b"},
    "Rushing Falls": {"Babbling Brook": "a", "Slippery Path": "b"},
    "Slippery Path": {"Rushing Falls": "a", "Rushing Pool": "b"},
    "Rushing Pool": {"Slippery Path": "a"},
    "Dark Tunnel": {"Rushing Pool": "a", "Darker Tunnel": "b"},
    "Darker Tunnel": {"Dark Tunnel": "a", "Darkest Tunnel": "b"},
    "Darkest Tunnel": {"Darker Tunnel": "a", "Cave's End": "b"},
    "Cave's End": {"Darkest Tunnel": "a"},
    "Lake Shore": {"Forest Gate": "a", "Lake Hobs - West": "b"},
    "Lake Hobs - West": {"Lake Shore": "a", "Lake Hobs": "b"},
    "Lake Hobs": {"Lake Hobs - West": "a", "Lake Hobs - East": "b"},
    "Lake Hobs - East": {"Lake Hobs": "a", "East Shore": "b"},
    "East Shore": {"Lake Hobs - East": "a", "Dusty Road": "b"},
    "Dusty Road": {"East Shore": "a", "Stony Gate": "b"},
    "Stony Gate": {"Dusty Road": "a", "Empty Square": "b"},
    "Empty Square": {"Stony Gate": "a", "Tyr's Battle Gear": "b",
                     "Happy Face Inn": "c", "Thura's Magic Shop": "d"},
    "Tyr's Battle Gear": {"Empty Square": "a"},
    "Happy Face Inn": {"Empty Square": "a"},
    "Thura's Magic Shop": {"Empty Square": "a"},
    "Dusty Road - East": {"Empty Square": "a", "Crossroads": "b"},
    "Empty Square": {"Stony Gate": "a", "Tyr's Battle Gear": "b",
                     "Happy Face Inn": "c", "Thura's Magic Shop": "d",
                     "Dusty Road - East": "e"},
    "Crossroads": {"Dusty Road - East": "a", "North Road": "b", "East Road": "c"},
    "North Road": {"Crossroads": "a"},
    "Brambles": {"North Road": "a"},
    "East Road": {"Crossroads": "a", "Castle Wall": "b"},
    "Castle Wall": {"East Road": "a", "Garden Path": "b"},
    "Garden Path": {"Castle Wall": "a", "Castle Keep": "b"},
    "Castle Keep": {"Garden Path": "a", "Entrance Hall": "b"},
    "Entrance Hall": {"Castle Keep": "a"},
    "Passage": {"Entrance Hall": "a"},
}

# ===== START =====
send(" ")  # Skip freeware notice -> already at Tree Square
print("=== START ===")
pinfo()

# ===== BUY GEAR =====
print("\n=== Shopping ===")
nav("Grubby's Armory", pm)
send("t"); send("a"); send("t"); send("d"); send("x")
nav("Tree Square", pm)
nav("Mura's Magic Shop", pm)
send("t"); send("a"); send("x")
nav("Tree Square", pm)
pinfo()

# ===== GRIND FOREST =====
print("\n=== Forest Grind ===")
nav("Forest Path", pm)
grind(25)
pinfo()

# Tall Tree gold
print("\n=== Tall Tree ===")
nav("Tall Tree", pm)
print(f"  {send('s')[:80]}")

# ===== DARK TUNNEL =====
print("\n=== Dark Tunnel ===")
for d in ["Forest Path","Bear Rock","Shadowy Path","Big Oaks","Meadow",
          "Babbling Brook","Rushing Falls","Slippery Path","Rushing Pool"]:
    nav(d, pm)
send("s")  # waterfall -> Dark Tunnel
print(f"  At: {get_loc()}")

# Grind caves
print("\n=== Cave Grind ===")
grind(30)
pinfo()

# Heal Major
nav("Darker Tunnel", pm); nav("Darkest Tunnel", pm)
print(f"  {send('s')[:80]}")

# ===== BLUE DRAGON =====
print("\n=== Blue Dragon ===")
nav("Cave's End", pm)
print(f"  {send('s')[:120]}")
print(f"  {fight()[:120]}")
pinfo()

# ===== BACK TO TOWN =====
print("\n=== Return & Rest ===")
for d in ["Darkest Tunnel","Darker Tunnel","Dark Tunnel","Rushing Pool",
          "Slippery Path","Rushing Falls","Babbling Brook","Meadow",
          "Big Oaks","Shadowy Path","Bear Rock","Forest Path",
          "Forest Gate","Tree Square"]:
    nav(d, pm)
nav("Squirrel's Nook Inn", pm)
send("t"); send("y")  # rest
pinfo()

# ===== CROSS LAKE =====
print("\n=== Cross Lake ===")
nav("Tree Square", pm); nav("Forest Gate", pm); nav("Lake Shore", pm)
move_to("b")  # Lake Hobs - West (works with water shoes)
print(f"  At: {get_loc()}")

# ===== SERPENT =====
print("\n=== Serpent ===")
nav("Lake Hobs", pm)
out = send("s")
print(f"  {out[:120]}")
if is_battle(out): fight()
pinfo()

# ===== EAST SIDE =====
print("\n=== East Side ===")
for d in ["Lake Hobs - East","East Shore","Dusty Road","Stony Gate","Empty Square"]:
    nav(d, pm)

# Shop
nav("Tyr's Battle Gear", pm)
s = pinfo()
send("t")
if s["gold"] >= 1000: send("b"); print("  Silver Sword!")
elif s["gold"] >= 260: send("a"); print("  2-Hand!")
send("x")
send("t")
if s["gold"] >= 850: send("d"); print("  Iron Plate!")
elif s["gold"] >= 300: send("c"); print("  Copper Plate!")
send("x")

nav("Empty Square", pm)
nav("Thura's Magic Shop", pm)
s = pinfo()
send("t")
if s["gold"] >= 400: send("a"); print("  Sleep!")
if s["gold"] >= 950: send("b"); print("  HealMajor!")
send("x")

# Grind
print("\n=== East Grind ===")
nav("Empty Square", pm)
nav("Dusty Road - East", pm)
nav("Crossroads", pm)
grind(30)
pinfo()

# Skull key
print("\n=== Skull Key ===")
print(f"  {send('s')[:80]}")

# ===== RED DRAGON =====
print("\n=== Red Dragon ===")
nav("North Road", pm)
out = send("m")
if "Where?" in out: send("a")  # Brambles
print(f"  {send('s')[:120]}")
if is_battle(): print(f"  {fight()[:120]}")
pinfo()

# More grinding
print("\n=== Castle Prep ===")
nav("Crossroads", pm)
grind(40)
pinfo()

# ===== CASTLE =====
print("\n=== Castle ===")
for d in ["East Road","Castle Wall","Garden Path","Castle Keep","Entrance Hall"]:
    nav(d, pm)
print(f"  {send('s')[:80]}")  # bookcase

grind(20)
pinfo()

# Find Throne Room
print("\n=== Throne Room ===")
for _ in range(15):
    stabilize()
    loc = get_loc()
    if "Throne" in loc: break
    out = send("m")
    if "Where?" in out:
        send("b")
        if is_battle(): fight()
    pinfo()

# ===== BAD KING =====
print("\n=== BAD KING ===")
print(f"  {send('t')[:200]}")
result = fight()
print(f"  {result[:300]}")
pinfo()
