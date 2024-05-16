from PIL import Image

# Open the text file containing RGB data
with open("rgb.txt", "r") as file:
    lines = file.readlines()

# Extract image dimensions (assuming first line has width and height)
width, height = (800, 525)

# Create an image object from the RGB list
image = Image.new("RGB", (width, height))

# Parse each line and extract RGB values
for line in lines:
    nox = line.replace("x","40")
    x, y, r, g, b = map(int, nox.split(","))  # Split each line by comma and convert to integers
    pixel = (r,g,b)
    #print(x,y,r,g,b)
    image.putpixel((x, y), pixel)

image.show()
