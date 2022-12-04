/*

    Test this with Valgrind to verify that allocs and frees 
    are proiperly matched, and be sure to compare float data[x] 
    with double data[x] with uint8_t data[x] etc.

*/

#include "read_idx.hpp"

int main(void) {

    const std::string filePaths[4] = {

        "../../mnist_data/train-images.idx3-ubyte",
        "../../mnist_data/train-labels.idx1-ubyte",
        "../../mnist_data/t10k-images.idx3-ubyte",
        "../../mnist_data/t10k-labels.idx1-ubyte"

    };

    int i;

    idx::Set<uint8_t, float>   * floatCastSet;
    idx::Set<uint8_t, double>  * doubleCastSet;
    idx::Set<uint8_t, uint8_t> * byteCastSet;
    
    std::cout << "Float-casting tests:" << std::endl;

    for (i = 0; i < 4; i++) {
        std::cout << "INIT NEW: <uint8_t, float>" << std::endl;
        floatCastSet = new idx::Set<uint8_t, float>(filePaths[i]);
        std::cout << "\t'--- [!] deleting from [ " << &floatCastSet << " ]\n" << std::endl;
        delete floatCastSet;
    }

    std::cout << "Double-casting tests:" << std::endl;

    for (i = 0; i < 4; i++) {
        doubleCastSet = new idx::Set<uint8_t, double>(filePaths[i]);
        std::cout << "\t'--- [!] deleting from [ " << &doubleCastSet << " ]\n" << std::endl;
        delete doubleCastSet;
    }

    std::cout << "Byte-casting tests:" << std::endl;

    for (i = 0; i < 4; i++) {
        byteCastSet = new idx::Set<uint8_t, uint8_t>(filePaths[i]);
        std::cout << "\t'--- [!] deleting from [ " << &byteCastSet << " ]\n" << std::endl;
        delete byteCastSet;
    }
    
    return 0;

}