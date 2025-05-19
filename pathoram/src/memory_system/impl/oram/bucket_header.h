#ifndef BUCKET_HEADER_H
#define BUCKET_HEADER_H

#include <map>

namespace Ramulator{

class BucketHeader {
    int bucket_id; // bucket ID
    unsigned char mask_dummy = 0;
    unsigned char mask_data = 0;
    unsigned char mask_empty = 0xFF;
    std::map<unsigned char, int> block_ids;//FIXME: Easier data structure can be used (just the [] operator is being used) (vector?)

    public:
        BucketHeader(int id) : bucket_id(id) {
            for(int i=0; i<8; i++){
                block_ids[i] = -1;
            }
        }
        std::map<unsigned char, int> get_block_ids() {
            return block_ids;
        }

        void insert_data_block(int offset, int block_id) {
            set_block_id(offset, block_id++);
            set_data(offset);
        }

        void insert_dummy_block(int offset) {
            set_block_id(offset, -1);
            set_dummy(offset);
        }

        void set_block_id(int offset, int block_id) {
            block_ids[offset] = block_id;
        }

        void set_dummy(int block_offset) {
            mask_dummy |= (1U << block_offset);
            mask_data &= ~(1U << block_offset);
            mask_empty &= ~(1U << block_offset);
        }

        void set_data(int block_offset) {            
            mask_dummy &= ~(1U << block_offset);
            mask_data |= (1U << block_offset);
            mask_empty &= ~(1U << block_offset);
        }

        void set_empty(int block_offset) {            
            mask_dummy &= ~(1U << block_offset);
            mask_data &= ~(1U << block_offset);
            mask_empty |= (1U << block_offset);
        }

        bool is_dummy(int block_offset) {
            return (mask_dummy & (1 << block_offset)) != 0;
        }

        bool is_data(int block_offset) {
            return (mask_data & (1 << block_offset)) != 0;
        }

        bool is_empty(int block_offset) {
            return (mask_empty & (1 << block_offset)) != 0;
        }

        void reset_masks() {
            mask_dummy = 0;
            mask_data = 0;
            mask_empty = 0xFF;
        }

        void print_bucket_header() {
            printf("BucketHeader {\n");
            printf("  bucket_id: %d\n", bucket_id);
            printf("  mask_dummy: 0x%02X\n", mask_dummy);
            printf("  mask_data:  0x%02X\n", mask_data);
            printf("  mask_empty: 0x%02X\n", mask_empty);
            
            printf("  block_ids:\n");
            for (const auto& [key, val] : block_ids) {
                printf("    0x%02X: %d\n", key, val);
            }
            printf("}\n");
        }
};

}

#endif   // BUCKET_HEADER_H