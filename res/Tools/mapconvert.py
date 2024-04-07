import json

def read_json_tilemap(json_file):
    with open(json_file, 'r') as f:
        data = json.load(f)
    tilemap = data['map']
    return tilemap

def generate_c_array(tile_indices, width, height):
    c_array = "int tile_data[{}][{}] = {{\n".format(height, width)
    for row_index in range(height):
        c_array += "    {"
        for col_index in range(width):
            index = row_index * width + col_index
            c_array += str(tile_indices[index])
            if col_index != width - 1:
                c_array += ", "
        c_array += "}"
        if row_index != height - 1:
            c_array += ","
        c_array += "\n"
    c_array += "};"
    return c_array

def write_c_array_to_file(c_array, output_file):
    with open(output_file, 'w') as f:
        f.write(c_array)

# Example usage
json_file = r'F:\AirshipAce\res\Background\map.json'
output_file = 'tilemap_array.c'

tile_indices = read_json_tilemap(json_file)
width = 84  # Width of the tilemap
height = 74  # Height of the tilemap
c_array = generate_c_array(tile_indices, width, height)

write_c_array_to_file(c_array, output_file)
