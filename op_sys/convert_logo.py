from PIL import Image
import sys
import os

if not os.path.exists('logo.png'):
    print("ERROR: logo.png not found")
    sys.exit(1)

img = Image.open('logo.png')
img = img.convert('RGBA')
img.thumbnail((250, 250), Image.Resampling.LANCZOS)
width, height = img.size

with open('kernel/src/logo.h', 'w') as f:
    f.write('#pragma once\n')
    f.write('#include <stdint.h>\n\n')
    f.write('namespace boot {\n')
    f.write(f'constexpr int LOGO_WIDTH = {width};\n')
    f.write(f'constexpr int LOGO_HEIGHT = {height};\n')
    f.write('const uint32_t LOGO_PIXELS[] = {\n')
    
    pixels = list(img.getdata())
    for i, p in enumerate(pixels):
        r, g, b, a = p
        if a < 128:
            # Transparent magic key
            hex_color = "0xFF00FF"
        else:
            hex_color = f"0x{(r << 16) | (g << 8) | b:06X}"
        f.write(f'{hex_color}, ')
        if (i + 1) % width == 0:
            f.write('\n')
            
    f.write('};\n}\n')

print(f"SUCCESS: converted {width}x{height} logo to kernel/src/logo.h")
