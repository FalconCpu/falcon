from PIL import Image
import numpy as np

def get_palette_from_hex_file(filename):
  """
  Reads a file containing 24-bit hex color codes and returns a Pillow palette.

  Args:
      filename (str): Path to the file containing hex color codes.

  Returns:
      PIL.Palette: A Pillow palette object containing the parsed colors.
  """
  palette = []
  with open(filename, "r") as f:
    for line in f:
      # Remove any leading/trailing whitespaces
      hex_code = line.strip()
      # Validate hex code format (6 characters)
      if len(hex_code) != 6 or not all(c in "0123456789abcdef" for c in hex_code):
        raise ValueError(f"Invalid hex code found in file: {hex_code}")
      # Convert hex code to RGB tuple
      #rgb_value = (int(hex_code[:2], 16), int(hex_code[2:4], 16), int(hex_code[4:], 16))
      palette.append(int(hex_code[:2], 16))
      palette.append(int(hex_code[2:4], 16))
      palette.append(int(hex_code[4:], 16))
      #palette.append(rgb_value)
  #return Image.Palette.new("RGB", palette)
  return palette


palette = get_palette_from_hex_file("/mnt/c/Users/simon/f32/rtl/palette.hex")
p = Image.new("P",(2,2))
p.putpalette(palette)


# print(palette)

im = Image.open("/mnt/c/Users/simon/OneDrive/Pictures/Camera Roll/14Aug22/IMG_2432.JPG")
print(im.format, im.size, im.mode)


new_image = im.rotate(0).resize((640,480)).quantize(palette=p, dither=Image.Dither.FLOYDSTEINBERG)
as_bytes = new_image.getdata()

with open("image.bin","wb") as f:
    for value in as_bytes:
      f.write(value.to_bytes(1, byteorder='little'))  # Example: write 1-byte integers (little-endian)


print(new_image.format, new_image.size, new_image.mode)

print(new_image.palette.colors)
a = np.asarray(new_image)
print(a)



new_image.show()
#im.show()