#!/usr/bin/env python3

import os

def check_rim_info():
    """Check rim detection for all pottery pieces"""
    
    breakline_dir = "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250905_ProperSegmentation/SfS_pp/Breaklines/"
    
    pieces_info = {}
    
    for piece_num in range(1, 9):
        piece_name = f"Pot_A_Piece_{piece_num:02d}"
        breakline_file = os.path.join(breakline_dir, f"{piece_name}_Breakline_0.pcd")
        
        if os.path.exists(breakline_file):
            try:
                with open(breakline_file, 'r') as f:
                    lines = f.readlines()
                
                # Find the header line with info_index (format: "# segments total_points info_index")
                header_line = None
                for line in lines:
                    if line.startswith("# ") and len(line.strip().split()) >= 4:
                        parts = line.strip().split()
                        if parts[0] == "#" and parts[1].isdigit() and parts[2].isdigit():
                            header_line = line.strip()
                            break
                
                if header_line:
                    parts = header_line.split()
                    segments = int(parts[1])
                    total_points = int(parts[2]) 
                    info_index = int(parts[3])
                    
                    # Apply SFS rim detection logic
                    is_seg_rim = (abs(info_index) == 1 or abs(info_index) == 3)
                    is_seg_base = (abs(info_index) == 2 or abs(info_index) == 3)
                    
                    pieces_info[piece_num] = {
                        'segments': segments,
                        'points': total_points,
                        'info_index': info_index,
                        'is_rim': is_seg_rim,
                        'is_base': is_seg_base,
                        'header': header_line
                    }
                    
                else:
                    print(f"❌ {piece_name}: Could not find valid header line")
                    
            except Exception as e:
                print(f"❌ {piece_name}: Error reading file: {e}")
        else:
            print(f"❌ {piece_name}: File not found")
    
    # Analyze results
    print("RIM DETECTION ANALYSIS")
    print("=" * 50)
    
    piece_colors = {1: "Red", 2: "Green", 3: "Blue", 4: "Yellow", 5: "Magenta", 6: "Cyan", 7: "Orange", 8: "Purple"}
    
    rim_pieces = []
    base_pieces = []
    neither_pieces = []
    
    for piece_num in sorted(pieces_info.keys()):
        info = pieces_info[piece_num]
        color = piece_colors.get(piece_num, "Unknown")
        
        rim_status = "🔴 RIM" if info['is_rim'] else ""
        base_status = "🟤 BASE" if info['is_base'] else ""
        status = f"{rim_status} {base_status}".strip() or "⚪ NEITHER"
        
        print(f"Piece {piece_num} ({color:<7}): info_index={info['info_index']:2d} → {status}")
        print(f"  Header: {info['header']}")
        
        if info['is_rim']:
            rim_pieces.append((piece_num, color))
        if info['is_base']:
            base_pieces.append((piece_num, color))
        if not info['is_rim'] and not info['is_base']:
            neither_pieces.append((piece_num, color))
    
    print(f"\nSUMMARY:")
    print(f"🔴 RIM PIECES ({len(rim_pieces)}): {', '.join([f'{p}({c})' for p, c in rim_pieces])}")
    print(f"🟤 BASE PIECES ({len(base_pieces)}): {', '.join([f'{p}({c})' for p, c in base_pieces])}")  
    print(f"⚪ NEITHER ({len(neither_pieces)}): {', '.join([f'{p}({c})' for p, c in neither_pieces])}")
    
    # Check reliability
    print(f"\nRIM DETECTION RELIABILITY ANALYSIS:")
    print(f"Total pieces analyzed: {len(pieces_info)}")
    
    if len(pieces_info) == 8:
        print("✅ All 8 pieces have breakline data")
        
        if len(rim_pieces) == 0:
            print("❌ CRITICAL: No rim pieces detected!")
            print("   This means info_index values are all 0, 2, or -2")
            print("   Rim detection is NOT working - all pieces treated as body fragments")
        elif len(rim_pieces) < 3:
            print(f"⚠️  WARNING: Only {len(rim_pieces)} rim pieces detected")  
            print("   Expected 2-4 rim pieces for a pottery vessel")
        else:
            print(f"✅ {len(rim_pieces)} rim pieces detected - reasonable for pottery")
            
        # Check the problematic blue/green pieces specifically
        blue_piece = pieces_info.get(3)  # Piece 3 = Blue
        green_piece = pieces_info.get(2) # Piece 2 = Green
        
        if blue_piece and green_piece:
            print(f"\nPROBLEMATIC PIECES ANALYSIS:")
            print(f"Blue (Piece 3):  info_index={blue_piece['info_index']}, is_rim={blue_piece['is_rim']}")
            print(f"Green (Piece 2): info_index={green_piece['info_index']}, is_rim={green_piece['is_rim']}")
            
            if blue_piece['is_rim'] or green_piece['is_rim']:
                print("🔍 HYPOTHESIS CONFIRMED: Blue/Green pieces have rim detection")
                print("   Problem: Rim pieces positioned upside-down due to missing orientation constraints")
            else:
                print("🔍 HYPOTHESIS REJECTED: Blue/Green pieces are NOT rim pieces")
                print("   Problem: Intersection issue is not related to rim orientation")
    else:
        print(f"❌ Only {len(pieces_info)}/8 pieces analyzed - incomplete data")

if __name__ == "__main__":
    check_rim_info()