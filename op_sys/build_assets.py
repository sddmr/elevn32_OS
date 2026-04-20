import os
from PIL import Image, ImageDraw

def create_placeholder(filename, color, text):
    img = Image.new('RGBA', (128, 128), color=(0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    # Draw rounded rectangle (approximate by drawing circle + rectangles)
    d.rounded_rectangle([10, 10, 118, 118], radius=20, fill=color)
    # Add text
    d.text((32, 56), text, fill="white")
    img.save(filename)

# Ensure icons exist
os.makedirs('icons', exist_ok=True)
if not os.path.exists('icons/settings.png'):
    create_placeholder('icons/settings.png', (100, 100, 100, 255), "SET")
if not os.path.exists('icons/shell.png'):
    create_placeholder('icons/shell.png', (40, 40, 40, 255), ">_")
if not os.path.exists('icons/info.png'):
    create_placeholder('icons/info.png', (0, 120, 255, 255), "INFO")

with open('kernel/src/assets.h', 'w') as f:
    f.write('#pragma once\n#include <stdint.h>\n\nnamespace assets {\n')
    
    # Process small logo
    if os.path.exists('logo.png'):
        img = Image.open('logo.png').convert('RGBA')
        img.thumbnail((18, 18), Image.Resampling.LANCZOS)
        w, h = img.size
        f.write(f'constexpr int LOGO_SMALL_W = {w};\n')
        f.write(f'constexpr int LOGO_SMALL_H = {h};\n')
        f.write('const uint32_t LOGO_SMALL[] = {\n')
        for i, p in enumerate(list(img.getdata())):
            r, g, b, a = p
            if a < 128:
                f.write('0xFF00FF, ')
            else:
                f.write(f'0x{(r<<16)|(g<<8)|b:06X}, ')
        f.write('\n};\n\n')

    # Process icons
    icons = ['settings', 'shell', 'info']
    f.write(f'constexpr int ICON_SIZE = 48;\n')
    for icon in icons:
        img = Image.open(f'icons/{icon}.png').convert('RGBA')
        img = img.resize((48, 48), Image.Resampling.LANCZOS)
        f.write(f'const uint32_t ICON_{icon.upper()}[] = {{\n')
        for i, p in enumerate(list(img.getdata())):
            r, g, b, a = p
            if a < 128:
                f.write('0xFF00FF, ')
            else:
                f.write(f'0x{(r<<16)|(g<<8)|b:06X}, ')
        f.write('\n};\n\n')
        
    f.write('}\n')

print("Assets compiled successfully!")
