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
void VXL_set_voxel(struct FbVoxelWorld *world, int x, int y, int z, struct FbVoxel voxel);
struct FbVoxel VXL_find_voxel(struct FbVoxelWorld *world, int x, int y, int z);
