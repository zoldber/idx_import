#include "read_idx.hpp"

#define TRAIN_SET_DIR "../../mnist_data/train-images.idx3-ubyte"

#define READ_TYPE unsigned char

#define CAST_TYPE float

int main(void) {

    // creates new CAST_TYPE ** data memory block
    auto castDemo = new idx::Set<READ_TYPE, CAST_TYPE>(TRAIN_SET_DIR);

    auto setDims = castDemo->dims();

    uint32_t i, I = std::get<0>(setDims);
    uint32_t r, R = std::get<1>(setDims);
    uint32_t c, C = std::get<2>(setDims);

    // number of images to be printed to terminal
    const uint32_t demoSize = 3;

    for (i = 0; i < demoSize; i++) {

        for (r = 0; r < R; r++) {

            for (c = 0; c < C; c++) {

                // print pixel if darker than 50%
                printf((castDemo->item(i)[(r * C) + c] > 0x80) ? "#" : " ");

            }

            puts("");

        }

        std::cout << "\n - END DEMO ITEM ( " << i + 1 << " / " << demoSize << " ) -\n" << std::endl;

    }

    return 0;

}