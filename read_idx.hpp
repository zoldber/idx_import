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
    template<typename readType>
    class AutoEndianBuffer {

        private:

            size_t i, j;

            // endianness of machine set once in constructor
            bool isLE;

        public:

            union { 
                
                readType joined; 
                char bytes[sizeof(readType)]; 
                
            } reg;

            AutoEndianBuffer(void) {

                // perform endianness test on init. This also sets BE to true
                // (and thus skips byte-swapping) if readType is a single byte
                reg.joined = 0x01;
                isLE = reg.bytes[0];

            }

            // note: reconsider use of memcpy() which might not be as performant 
            // as a direct assignment)
            void writeTo(readType * dest) {

                if (isLE) {

                    for (i = 0, j = sizeof(readType) - 1; i < j; ++i, --j) {

                        reg.bytes[i] ^= reg.bytes[j];
                        reg.bytes[j] ^= reg.bytes[i];
                        reg.bytes[i] ^= reg.bytes[j];
                    }

                }

                std::memcpy(dest, &reg.joined, sizeof(readType));

                return;

            }


    };

    template<typename readType, typename castType>
    class Set {

        private:

            uint32_t magicNumber;

            // given I, R, C = item, row, col counts respectively, and for path suffix:
            // ".idxN", any set is represented in 3 dims, with the following precedence
            //
            // for N = 1 (a set of 1x1 items): data = [0:I][0:0]
            // for N = 2 (a set of 1xC items): data = [0:I][0:C]
            // for N = 3 (a set of RxC items): data = [0:I][0:R*C]
            uint32_t I, R, C;

            // dimension order: { I, R, C }
            std::tuple<uint32_t, uint32_t, uint32_t> dimensions;

            // uint32_t type used for MNIST header (idx format is nonstandard but most references describe this as one)
            AutoEndianBuffer<uint32_t> * buff32;

            // only created if read-type has endianness (i.e. exceeds one byte in size)
            AutoEndianBuffer<readType> * formattedBuffer;

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

            // public, as original read data need not be preserved or protected
            castType ** data;

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
                buff32 = new AutoEndianBuffer<uint32_t>();

                // all N!=0 idxN files will have u32 magicNum, u32 numItems
                //  N = 2 will have u32numCols (numItems # of arrays)
                //  N = 3 will have u32numRows, u32numCols (numItems # of matrices)
                file.read(buff32->reg.bytes, sizeof(uint32_t));
                buff32->writeTo(&magicNumber);

                // base-case dimensions of an idx_ structure
                I = 1;
                R = 1;
                C = 1;

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

                const unsigned int itemSize = R * C;

                uint32_t i, j;

                // The most ubiquitous MNIST collection seems uses uchar data exclusively (hence no endianness beyond
                // file header) to store pixel values in each item; a separate read routine is included anyway in the
                // event that an altered (or custom) file header specifies multi-byte data
                if (sizeof(readType) == sizeof(char)) {

                    // the file.read() method expects a target arg of type 'char *__s', and template-casting for an
                    // unsigned input was either flagged by the compiler or resulted in sporadic behavior for unit-
                    // tests of varied casting types. An aliased buffer solves this with minimal complexity
                    union { char byte; readType cbyte; } reg;

                    for (i = 0; i < I; i++) {

                        data[i] = new castType[itemSize];

                        for (j = 0; j < itemSize; j++) {

                            file.read(&reg.byte, sizeof(readType));

                            data[i][j] = reg.cbyte;

                        }

                    }

                } else {

                    formattedBuffer = new AutoEndianBuffer<readType>();

                    for (i = 0; i < I; i++) {

                        data[i] = new castType[itemSize];

                        for (j = 0; j < itemSize; j++) {

                            file.read(formattedBuffer->reg.bytes, sizeof(readType));

                            data[i][j] = (readType)(formattedBuffer->reg.joined);

                        }

                    }


                }

                size_t allocSize = I * R * C * sizeof(readType);

                const std::string sizes[4] = { " bytes", " KB", " MB", " GB" };

                for (i = 0; (i < sizes->size()) && (allocSize / 1000); i++) {

                    allocSize /= 1000;

                }

                std::cout << " '--- allocated " << allocSize << sizes[i];
                std::cout << " for " << I << " [ " << R << " x " << C << " ] items."<< std::endl;

                return;

            }

            ~Set(void) {

                delete buff32;

                delete formattedBuffer;
                
                for (size_t item = 0; item < I; item++) {

                    delete data[item];

                }

                delete [] data;

                return;

            }

            // { numItems, R, C }
            std::tuple<uint32_t, uint32_t, uint32_t> dims(void) {

                return dimensions;

            }

    };
}