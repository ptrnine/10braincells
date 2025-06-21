#include <catch2/catch_test_macros.hpp>

#include <pugixml.hpp>

#include <core/io/out.hpp>

#include <codegen/vulkan/commands.hpp>
#include <codegen/vulkan/class.hpp>

TEST_CASE("vk_codegen_member_functions") {
    constexpr std::string_view registry = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <registry>
            <commands>
                <command api="vulkan" successcodes="VK_SUCCESS,VK_INCOMPLETE" errorcodes="VK_ERROR_OUT_OF_HOST_MEMORY,VK_ERROR_OUT_OF_DEVICE_MEMORY">
                    <proto><type>VkResult</type> <name>vkMemberEnumerate</name></proto>
                    <param><type>VkObject</type> <name>object</name></param>
                    <param optional="false,true"><type>uint32_t</type>* <name>pPropertyCount</name></param>
                    <param optional="true" len="pPropertyCount"><type>VkLayerProperties</type>* <name>pProperties</name></param>
                </command>
                <command api="vulkan" successcodes="VK_SUCCESS,VK_INCOMPLETE" errorcodes="VK_ERROR_OUT_OF_HOST_MEMORY,VK_ERROR_OUT_OF_DEVICE_MEMORY">
                    <proto><type>void</type> <name>vkMemberAcceptSpan</name></proto>
                    <param><type>VkObject</type> <name>object</name></param>
                    <param><type>uint32_t</type> <name>pPropertyCount</name></param>
                    <param len="pPropertyCount">const <type>VkLayerProperties</type>* <name>pProperties</name></param>
                </command>
                <command api="vulkan">
                    <proto><type>void</type> <name>vkMemberAcceptSpan2</name></proto>
                    <param><type>VkObject</type> <name>object</name></param>
                    <param><type>uint32_t</type> <name>pPropertyCount</name></param>
                    <param len="pPropertyCount">const <type>VkLayerProperties</type>* <name>pProperties</name></param>
                </command>
            </commands>
        </registry>)";


    pugi::xml_document doc;
    doc.load_buffer(registry.data(), registry.size());
    auto cmds = cg::vk::parse_commands(doc.child("registry"));

    cg::vk::class_instance_dependent c;
    c.instance.name = "inst";
    c.instance.type = "instance_t";
    c.handle.name = "obj";
    c.handle.type = "vk::object";
    c.name = "object_t";
    c.functions     = {
        {"member_enumerate"},
        {"member_accept_span"},
        {"member_accept_span2"},
    };

    core::io::out out{std::string{}};
    cg::vk::external_generated eg;
    c.generate(out, cmds, eg);
    out.flush();

    /* TODO: IMPLEMENT ME */
    std::cerr << "BUFF: " << out.base_buff() << std::endl;
}
