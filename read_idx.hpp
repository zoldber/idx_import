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

    // note: the NYU hosted archive of the MNIST set is big-endian
    // and at least a idx few header values are multi-byte, requiring
    // formatting when reading. The reading / writing methods below
    // make no assumption regarding architecture of the host machine
    // nor the size of each datum within each item extracted (e.g.
    // an idx file with data exceeding a byte might be imported).
    template<typename dataType>
    class AutoEndianBuffer {

        private:

            size_t i, j;

            // endianness of machine set once in constructor
            bool isLE;

        public:

            union { 
                
                dataType joined; 
                char bytes[sizeof(dataType)]; 
                
            } reg;

            AutoEndianBuffer(void) {

                // perform endianness test on init. This also sets BE to true
                // (and thus skips byte-swapping) if dataType is a single byte
                reg.joined = 0x01;
                isLE = reg.bytes[0];

            }

            // note: reconsider use of memcpy() (this is a proxy'd system 
            // call that might not be as performant as direct assignment)
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

    template<typename dataType, typename castType>
    class Set {

        private:

            // Set data can be represented this way for N = 1-3
            // given I = item count, R = item row count, C = item col count
            //
            // for N = 1 (a set of 1x1 items): data = [0:I][0:0]
            // for N = 2 (a set of 1xC items): data = [0:I][0:C]
            // for N = 3 (a set of RxC items): data = [0:I][0:R*C]

            castType ** data;

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

                // header will contain u32 type values in need of endianess formatting
                auto buff32 = new AutoEndianBuffer<uint32_t>();

                // all N!=0 idxN files will have u32 magicNum, u32 numItems
                //  N = 2 will have u32numCols (numItems # of arrays)
                //  N = 3 will have u32numRows, u32numCols (numItems # of matrices)
                file.read(buff32->reg.bytes, sizeof(uint32_t));
                buff32->writeTo(&magicNumber);

                // base-case dimensions of an idx_ structure
                uint32_t I = 1;
                uint32_t R = 1;
                uint32_t C = 1;

                // set numItems
                file.read(buff32->reg.bytes, sizeof(uint32_t));
                buff32->writeTo(&I);

                // set numCols per item
                if (N > 1) {
                    file.read(buff32->reg.bytes, sizeof(uint32_t));
                    buff32->writeTo(&C);
                }

                // set numRows per item
                if (N > 2) {
                    file.read(buff32->reg.bytes, sizeof(uint32_t));
                    buff32->writeTo(&R);
                }

                // update Set dimensions after reading header
                dimensions = {I, R, C};

                // data is [I] items
                // an item is an [R * C] sized array
                data = new castType * [I];

                uint32_t i, j;

                const unsigned int itemSize = R * C;

                // the most ubiquitous MNIST seem to exclusively use uchar (hence no endianness beyond header)
                // to store pixel values in each item; in the event that this is not true, a seperate block is
                // present to initialize a new AutoEndianBuffer and format entries on little-endian machines.
                if (sizeof(dataType)==sizeof(char)) {

                    dataType buffer;

                    for (i = 0; i < I; i++) {

                        data[i] = new castType[itemSize];

                        for (j = 0; j < itemSize; j++) {

                            file.read((char *)buffer, sizeof(dataType));

                            data[i][j] = (castType)buffer;

                        }

                    }

                } else {

                    auto elemBuff = new AutoEndianBuffer<dataType>();

                    for (i = 0; i < I; i++) {

                        data[i] = new castType[itemSize];

                        for (j = 0; j < itemSize; j++) {

                            file.read(elemBuff->reg.bytes, sizeof(dataType));

                            data[i][j] = (castType)(elemBuff->joined); //type cast this regardless of input format

                        }

                    }


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

            castType * item(const unsigned int i) {

                return data[i];

            }

    };

}