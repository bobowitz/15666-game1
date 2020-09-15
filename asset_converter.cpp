#include <iostream>
#include <fstream>

#include <array>

#include "load_save_png.hpp"
#include "read_write_chunk.hpp"
#include "PPU466.hpp"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <spritesheet.png>" << std::endl;
        return 1;
    }

    const char *filename = argv[1];

    // sprite sheet objects
    glm::uvec2 ss_size;
    std::vector<glm::u8vec4> ss_data;

    load_png(std::string(filename), &ss_size, &ss_data, UpperLeftOrigin);

    std::ofstream asset_file;
    asset_file.open("asset_data", std::ios::binary);

    // load palettes (from upper left corner of the spritesheet png)
    const uint8_t NUM_PALETTES = 8;
    const uint8_t COLORS_PER_PALETTE = 4;

    std::array<glm::u8vec4, COLORS_PER_PALETTE * NUM_PALETTES> palettes;

    const uint8_t PALETTE_SCALE = 2; // scale of the palette on the png
    for (uint8_t y = 0; y < NUM_PALETTES; y++)
    {
        for (uint8_t x = 0; x < COLORS_PER_PALETTE; x++)
        {
            palettes[y * COLORS_PER_PALETTE + x] = ss_data[(y * ss_size.x + x) * PALETTE_SCALE];
        }
    }

    // load tiles
    std::array<PPU466::Tile, 16 * 16> tiles;

    for (int tile_x = 0; tile_x < 16; tile_x++)
    {
        for (int tile_y = 0; tile_y < 16; tile_y++)
        {
            int tile_index = tile_y * 16 + tile_x;
            for (int pixel_y = 0; pixel_y < 8; pixel_y++)
            {
                // clear the row
                tiles[tile_index].bit0[7 - pixel_y] = 0;
                tiles[tile_index].bit1[7 - pixel_y] = 0;
                for (int pixel_x = 0; pixel_x < 8; pixel_x++)
                {

                    int palette_col_offset = 1; // skip the first column (palette info)
                    int x = (tile_x + palette_col_offset) * 8 + pixel_x;
                    int y = tile_y * 8 + pixel_y;
                    int pixel_index = y * ss_size.x + x;

                    glm::u8vec4 color = ss_data[pixel_index];

                    if (glm::all(glm::equal(glm::u8vec4(0, 0, 0, 255), color)))
                    {
                        // 0b00
                    }
                    else if (glm::all(glm::equal(glm::u8vec4(84, 84, 84, 255), color)))
                    {
                        // 0b01
                        tiles[tile_index].bit0[7 - pixel_y] |= (1 << pixel_x);
                    }
                    else if (glm::all(glm::equal(glm::u8vec4(168, 168, 168, 255), color)))
                    {
                        // 0b10
                        tiles[tile_index].bit1[7 - pixel_y] |= (1 << pixel_x);
                    }
                    else if (glm::all(glm::equal(glm::u8vec4(255, 255, 255, 255), color)))
                    {
                        // 0b11
                        tiles[tile_index].bit0[7 - pixel_y] |= (1 << pixel_x);
                        tiles[tile_index].bit1[7 - pixel_y] |= (1 << pixel_x);
                    }
                    else
                    {
                        std::cerr << "Unrecognized color on tileset at (" << x << ", " << y << ") " << +color.x << "," << +color.y << "," << +color.z << std::endl;
                    }
                }
            }
        }
    }

    // write palette

    write_chunk<glm::u8vec4>("pals", std::vector<glm::u8vec4>(palettes.begin(), palettes.end()), &asset_file);

    // write tiles

    write_chunk<PPU466::Tile>("tile", std::vector<PPU466::Tile>(tiles.begin(), tiles.end()), &asset_file);

    asset_file.close();

    return 0;
}