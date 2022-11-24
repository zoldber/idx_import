// idx file format reference: 
//      https://deepai.org/dataset/mnist
//      archive: https://archive.ph/WFxMP

#include <iostream>
#include <fstream>
#include <vector>

// used for memcpy
#include <cstring>

// used for item dim, consider alternative
#include <tuple>

namespace idx {

    template<class dataType>
    class AutoEndianBuffer {

        private:

            size_t i, j;

            // endianness of machine set once in constructor
            bool isLE;

            union { 
                
                dataType joined; 
                char bytes[sizeof(dataType)]; 
                
            } reg;


        public:

            AutoEndianBuffer(void) {

                // perform endianness test on init.
                // this also sets BE to true (and 
                // thus skips byte swap) if dataType
                // is a one-byte type, like char
                reg.joined = 0x01;
                isLE = reg.bytes[0];

            }

            char * addr(void) {

                return reg.bytes;

            }

            void writeTo(dataType * dest) {

                if (isLE) {

                    for (i = 0, j = sizeof(dataType) - 1; i < j; ++i, --j) {

                        reg.bytes[i] ^= reg.bytes[j];
                        reg.bytes[j] ^= reg.bytes[i];
                        reg.bytes[i] ^= reg.bytes[j];
                    }

                }

                std::memcpy(dest, &reg.joined, sizeof(dataType));

                return;

            }


    };

    template<class dataType>
    class Set {

        private:

            // Set data can be represented this way for N = 1-3
            // given I = item count, R = item row count, C = item col count
            //
            // for N = 1 (a set of 1x1 items): data = [0:I][0:0]
            // for N = 2 (a set of 1xC items): data = [0:I][0:C]
            // for N = 3 (a set of RxC items): data = [0:I][0:R*C]

            dataType ** data;

            uint32_t magicNumber;

            // { numItems, R, C }
            std::tuple<uint32_t, uint32_t, uint32_t> dimensions;


            // returns N for -.idxN- in filePath, or 0 for invalid suffix
            // consider seperate error codes for return cases below
            int parseSuffix(const std::string& path) {

                const std::string minimalSuffix = ".idx";

                auto suffix = path.find(minimalSuffix);

                // chars from '.' are too few to be valid
                if ((path.size() - suffix) < (minimalSuffix.size() + 1)) { return 0; }

                // chars from '.' don't match .idx
                if (path.substr(suffix, minimalSuffix.size()).compare(minimalSuffix)){ return 0; }

                suffix += minimalSuffix.size();

                // returns 0 if value immediately following .idx is either 0 or non-numeric
                return std::atoi(&path[suffix]);
            }

        public:

            Set(const std::string filePath) {

                std::cout << "-+- reading: \"" << filePath << "\"" << std::endl;

                int N = parseSuffix(filePath);

                if ((N < 1) || (N > 3)) {

                    std::cout << " '--- parsed item dimension: " << N << ", exit failure" << std::endl;

                    exit(EXIT_FAILURE);

                }

                std::ifstream file;
                
                file.open(filePath, std::ios::in | std::ios::binary);

                if (!file.is_open()) {

                    std::cout << " '--- failed to open, exit failure" << std::endl;
                    
                    exit(EXIT_FAILURE);
                }

                auto buff32 = new AutoEndianBuffer<uint32_t>();

                // all N!=0 idxN files will have u32 magicNum, u32 numItems
                //  N = 2 will have u32numCols (numItems # of arrays)
                //  N = 3 will have u32numRows, u32numCols (numItems # of matrices)
                file.read(buff32->addr(), sizeof(uint32_t));
                buff32->writeTo(&magicNumber);

                // default dimensions of an idx structure
                uint32_t I = 1;
                uint32_t R = 1;
                uint32_t C = 1;

                // set numItems
                file.read(buff32->addr(), sizeof(uint32_t));
                buff32->writeTo(&I);

                // set numCols per item
                if (N > 1) {
                    file.read(buff32->addr(), sizeof(uint32_t));
                    buff32->writeTo(&C);
                }

                // set numRows per item
                if (N > 2) {
                    file.read(buff32->addr(), sizeof(uint32_t));
                    buff32->writeTo(&R);
                }

                // update Set dimensions after reading header
                dimensions = {I, R, C};

                // data is [I] items
                // an item is an [R * C] sized array
                data = new dataType * [I];

                //TODO: read-in data using AutoEndianBuffer<dataType>();
                //      this works now because the test data is uchar,
                //      but update for truly dynamic solution

                uint32_t i, j;

                const unsigned int itemSize = R * C;

                auto elemBuff = new AutoEndianBuffer<dataType>();

                for(i = 0; i < I; i++) {

                    data[i] = new dataType[itemSize];

                    // faster but less dynamic alternative
                    file.read((char *)data[i], itemSize * sizeof(dataType));

                    /*  Faster without calling dynamic buffer (MNIST set is all unsigned char hence no endinanness)

                    for (j = 0; j < itemSize; j++) {

                        file.read(elemBuff->addr(), sizeof(dataType));

                        elemBuff->writeTo(&data[i][j]);

                    }

                    */

                }

                size_t allocSize = I * R * C * sizeof(dataType);

                const std::string sizes[4] = { " bytes", " KB", " MB", " GB" };

                for (i = 0; (i < sizes->size()) && (allocSize / 1000); i++) {

                    allocSize /= 1000;

                }

                std::cout << " '--- allocated " << allocSize << sizes[i];
                std::cout << " for " << I << " [ " << R << " x " << C << " ] items.\n"<< std::endl;

                return;

            }

            std::tuple<uint32_t, uint32_t, uint32_t> dims(void) {

                return dimensions;

            }

            dataType * item(const unsigned int i) {

                return data[i];

            }

    };

}