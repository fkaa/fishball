#include "spirv.h"
#include "array.h"
#include "bal.h"
#include "file.h"
#include "export.h"
#include "toml.h"
#include "spirv_reflect.h"

struct BalSpirv *BAL_compile_glsl(struct BalExporter *exporter, const char *name, const char *outfile, const char *path, toml_table_t *options)
{
    const char *stage_str = 0;

    if (options && toml_rtos(toml_raw_in(options, "stage"), &stage_str)) {

    }

    enum BalShaderStage stage = 0;
    if (strcmp(stage_str, "vert") == 0) { 
        stage = BAL_SHADER_VERTEX;
    } else if (strcmp(stage_str, "frag") == 0) {
        stage = BAL_SHADER_PIXEL;
    }

    char compile_flags[256];
    snprintf(compile_flags, sizeof(compile_flags), "glslangValidator -V -s -S %s %s -o %s", stage_str, path, outfile);
    system(compile_flags);

    u32 len = 0;
    char *contents = 0;
    FILE_read_whole(outfile, &contents, &len);

    SpvReflectShaderModule module = {0};
    SpvReflectResult result = spvReflectCreateShaderModule(len, contents, &module);

    u32 count = 0;
    result = spvReflectEnumerateDescriptorSets(&module, &count, NULL);

    SpvReflectDescriptorSet **sets = malloc(count * sizeof(*sets));
    result = spvReflectEnumerateDescriptorSets(&module, &count, sets);

    VkDescriptorSetLayoutBinding *bindings = 0;

    for (u32 i = 0; i < count; ++i) {
        SpvReflectDescriptorSet *set = sets[i];

        for (u32 j = 0; j < set->binding_count; ++j) {
            SpvReflectDescriptorBinding *spv_bind = set->bindings[j];
            VkDescriptorSetLayoutBinding bind = { 0 };

            bind.binding = spv_bind->binding;
            bind.descriptorType = spv_bind->descriptor_type;
            bind.descriptorCount = 1;
            for (u32 k = 0; k < spv_bind->array.dims_count; ++k) {
                bind.descriptorCount *= spv_bind->array.dims[k];
            }

            ARRAY_push(bindings, bind);
        }
    }

    struct BalBuffer *desc_buffer = BAL_allocate_buffer(exporter, ARRAY_size(bindings) * sizeof(VkDescriptorSetLayoutBinding));
    desc_buffer->size = ARRAY_size(bindings);
    memcpy(desc_buffer->data, bindings, ARRAY_size(bindings) * sizeof(VkDescriptorSetLayoutBinding));

    struct BalSpirv *spirv = BAL_allocate_spirv(exporter, name, contents, len);
    spirv->module = 0;
    spirv->stage = stage;
    BAL_SET_REF_PTR(spirv->layout, desc_buffer);

    return spirv;
}
