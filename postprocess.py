#!/usr/bin/env python3
"""Post-process decompiled.c to add better function names based on analysis."""

import re

# Known function addresses and their names based on:
# 1. Symbol table exports from radare2
# 2. String references and patterns in the code
# 3. Windows API call patterns

function_map = {
    # Main program flow
    "1000:0200": "entry",  # Program entry point
    "1000:3430": "WINMAIN",  # Windows main
    
    # Game state management
    "1000:00ea": "FUN_1000_00ea_init_task_table",  # Task initialization loop
    "1000:012e": "FUN_1000_012e_cleanup",  # Cleanup
    
    # Location system
    "1000:0837": "print_location",  # Big switch for locations
    
    # Commands
    "1000:10ef": "search_locations",  # Search command handler
    "1000:12f6": "talk_locations",  # Talk command handler
    "1000:18a7": "magic_cast_spell",  # Magic/spell casting
    "1000:1bd1": "fight",  # Fight command
    "1000:1ce8": "enemy_turn",  # Enemy turn in combat
    
    # Character stats
    "1000:28d4": "status",  # Show status screen
    "1000:29ab": "random",  # Random number generator
    
    # Combat
    "1000:299b": "get_damage",  # Calculate damage
    
    # Items and shops
    "1000:2c46": "buy_weapon",  # Weapon shop
    "1000:2c4c": "buy_magic",  # Magic shop
    
    # File I/O
    "1000:6daa": "load_game",  # Load game
    "1000:6170": "save_me",  # Save game
    
    # Entry points from symbol table
    "1000:6918": "_EASYWINPROC",  # EasyWin procedure
}

# Read the decompiled file
with open('/home/thomas/code/badking/decompiled.c', 'r') as f:
    content = f.read()

# Process the content - add better comments
lines = content.split('\n')
output_lines = []

for line in lines:
    # Check if this is a function address comment
    addr_match = re.search(r'// Address: ([0-9a-fA-F:]+)', line)
    if addr_match:
        addr = addr_match.group(1)
        if addr in function_map:
            output_lines.append("// Identified as: %s" % function_map[addr])
    output_lines.append(line)

# Write the improved output
with open('/home/thomas/code/badking/decompiled.c', 'w') as f:
    f.write('\n'.join(output_lines))

print("Applied function name mappings to decompiled.c")
