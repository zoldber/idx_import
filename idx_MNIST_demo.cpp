#include "read_idx.hpp"

#define TRAIN_SET_DIR "../../mnist_data/train-images.idx3-ubyte"

int main(void) {

    // given a data storage format 'd', and desired cast-type 'c', template is:
    //      auto mySet = new idx::Set<d, c>("../data_path/data.idx3");
    auto demo = new idx::Set<unsigned char, float>(TRAIN_SET_DIR);

    auto setDims = demo->dims();

    uint32_t i, I = std::get<0>(setDims);
    uint32_t r, R = std::get<1>(setDims);
    uint32_t c, C = std::get<2>(setDims);

    // number of images to be printed to terminal
    const uint32_t demoSize = 3;

    for (i = 0; i < demoSize; i++) {

        for (r = 0; r < R; r++) {

            for (c = 0; c < C; c++) {

                // print pixel if darker than 50%
                putc(demo->data[i][(r * C) + c] > 128.0 ? '#' : ' ', stdout);

            }

            puts("");

        }

        std::cout << "\n - END DEMO ITEM ( " << i + 1 << " / " << demoSize << " ) -\n" << std::endl;

    }

    return 0;

}