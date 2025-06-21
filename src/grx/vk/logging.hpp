#pragma once

#include <grx/vk/extension_defines.hpp>

#include <grx/vk/enums.cg.hpp>
#include <grx/vk/flags.cg.hpp>
#include <grx/vk/structs.cg.hpp>
#include <grx/vk/types.cg.hpp>

#include <util/log.hpp>

namespace vk {
enum class log_severity {
    off = 0,
    error,
    warning,
    performance_warning,
    info,
    debug,
};

namespace details {
    util::log_level get_log_level(vk::debug_report_ext_flags flags) {
        if (flags.test(vk::debug_report_ext_flag::error_ext)) {
            return util::log_level::error;
        }
        else if (flags.test(vk::debug_report_ext_flag::warning_ext) || flags.test(vk::debug_report_ext_flag::performance_warning_ext)) {
            return util::log_level::warn;
        }
        else if (flags.test(vk::debug_report_ext_flag::information_ext)) {
            return util::log_level::info;
        }
        else {
            return util::log_level::debug;
        }
    }

    util::log_level get_log_level(vk::debug_utils_message_severity_ext_flags flags) {
        if (flags.test(vk::debug_utils_message_severity_ext_flag::error_ext)) {
            return util::log_level::error;
        }
        else if (flags.test(vk::debug_utils_message_severity_ext_flag::warning_ext)) {
            return util::log_level::warn;
        }
        else if (flags.test(vk::debug_utils_message_severity_ext_flag::info_ext)) {
            return util::log_level::info;
        }
        else {
            return util::log_level::debug;
        }
    }


    bool32_t logger_callback(util::logger&                    logger,
                             vk::debug_report_ext_flags       flags,
                             vk::debug_report_object_type_ext object_type,
                             core::u64                        object,
                             const char*                      p_layer_prefix,
                             const char*                      p_message) {
        logger.log(get_log_level(flags), "(report:{}:{}:{}): {}", to_string(object_type), object, p_layer_prefix, p_message);
        return true;
    }

    bool32_t debug_utils_callback(util::logger&                                  logger,
                                  debug_utils_message_severity_ext_flags         severity,
                                  debug_utils_message_type_ext_flags             type,
                                  const debug_utils_messenger_callback_data_ext* data) {
        logger.log(get_log_level(severity), "(validation:{}): {}", type, data->message);
        return true;
    }
} // namespace details

void log_dump(util::logger& logger, size_t indent, const physical_device_features& features) {
    auto ind = std::string(indent, ' ');

#define dump_feature(feat) \
    logger.info("{}{}: {}", ind, #feat, features.feat ? "on" : "off")

    dump_feature(robust_buffer_access);
    dump_feature(full_draw_index_uint32);
    dump_feature(image_cube_array);
    dump_feature(independent_blend);
    dump_feature(geometry_shader);
    dump_feature(tessellation_shader);
    dump_feature(sample_rate_shading);
    dump_feature(dual_src_blend);
    dump_feature(logic_op);
    dump_feature(multi_draw_indirect);
    dump_feature(draw_indirect_first_instance);
    dump_feature(depth_clamp);
    dump_feature(depth_bias_clamp);
    dump_feature(fill_mode_non_solid);
    dump_feature(depth_bounds);
    dump_feature(wide_lines);
    dump_feature(large_points);
    dump_feature(alpha_to_one);
    dump_feature(multi_viewport);
    dump_feature(sampler_anisotropy);
    dump_feature(texture_compression_etc2);
    dump_feature(texture_compression_astc__ldr);
    dump_feature(texture_compression_bc);
    dump_feature(occlusion_query_precise);
    dump_feature(pipeline_statistics_query);
    dump_feature(vertex_pipeline_stores_and_atomics);
    dump_feature(fragment_stores_and_atomics);
    dump_feature(shader_tessellation_and_geometry_point_size);
    dump_feature(shader_image_gather_extended);
    dump_feature(shader_storage_image_extended_formats);
    dump_feature(shader_storage_image_multisample);
    dump_feature(shader_storage_image_read_without_format);
    dump_feature(shader_storage_image_write_without_format);
    dump_feature(shader_uniform_buffer_array_dynamic_indexing);
    dump_feature(shader_sampled_image_array_dynamic_indexing);
    dump_feature(shader_storage_buffer_array_dynamic_indexing);
    dump_feature(shader_storage_image_array_dynamic_indexing);
    dump_feature(shader_clip_distance);
    dump_feature(shader_cull_distance);
    dump_feature(shader_float64);
    dump_feature(shader_int64);
    dump_feature(shader_int16);
    dump_feature(shader_resource_residency);
    dump_feature(shader_resource_min_lod);
    dump_feature(sparse_binding);
    dump_feature(sparse_residency_buffer);
    dump_feature(sparse_residency_image2d);
    dump_feature(sparse_residency_image3d);
    dump_feature(sparse_residency2_samples);
    dump_feature(sparse_residency4_samples);
    dump_feature(sparse_residency8_samples);
    dump_feature(sparse_residency16_samples);
    dump_feature(sparse_residency_aliased);
    dump_feature(variable_multisample_rate);
    dump_feature(inherited_queries);
#undef dump_feature
}
} // namespace vk
