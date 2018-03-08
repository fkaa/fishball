struct FbVoxelWorldConfig {
    int chunk_count_x;
    int chunk_count_y;
    int chunk_count_z;
};

struct FbVoxel {
    char type;
};

struct FbVoxelWorld;

void VXL_new_world(struct FbVoxelWorldConfig cfg, struct FbVoxelWorld **world);
struct FbVoxel VXL_find_voxel(struct FbVoxelWorld *world, int x, int y, int z);
