#include <catch2/catch_test_macros.hpp>
#include <core/xxhash.hpp>

/*
 * Known-answer vectors for XXH32/XXH64, taken from the upstream xxHash project
 * (tests/sanity_test_vectors.h). Vectors cover lengths 0..115 with seeds
 * 0 and 0x9E3779B1, hashed against a pseudorandom test buffer generated
 * exactly as upstream does (fill_test_buffer below).
 */
namespace {

using core::byte;

struct vec32 {
    core::u32 len;
    core::u32 seed;
    core::u32 result;
};

struct vec64 {
    core::u32 len;
    core::u32 seed;
    core::u64 result;
};

constexpr vec32 xxh32_vectors[] = {
    {    0u, 0x00000000u, 0x02CC5D05u },
    {    0u, 0x9E3779B1u, 0x36B78AE7u },
    {    1u, 0x00000000u, 0xCF65B03Eu },
    {    1u, 0x9E3779B1u, 0xB4545AA4u },
    {    2u, 0x00000000u, 0x1151BEE4u },
    {    2u, 0x9E3779B1u, 0x1EDB879Au },
    {    3u, 0x00000000u, 0xC23884F5u },
    {    3u, 0x9E3779B1u, 0x1A269947u },
    {    4u, 0x00000000u, 0xA9DE7CE9u },
    {    4u, 0x9E3779B1u, 0x2BAAFE83u },
    {    5u, 0x00000000u, 0xEB1734BBu },
    {    5u, 0x9E3779B1u, 0x5874DAB0u },
    {    6u, 0x00000000u, 0x659F0C97u },
    {    6u, 0x9E3779B1u, 0x0BCF25C5u },
    {    7u, 0x00000000u, 0x5E1056CDu },
    {    7u, 0x9E3779B1u, 0x3ED9D3FCu },
    {    8u, 0x00000000u, 0xA3F6F44Bu },
    {    8u, 0x9E3779B1u, 0xC2A8E239u },
    {    9u, 0x00000000u, 0xFFB82A24u },
    {    9u, 0x9E3779B1u, 0xD35632C6u },
    {   10u, 0x00000000u, 0xB1E5032Eu },
    {   10u, 0x9E3779B1u, 0x18679D60u },
    {   11u, 0x00000000u, 0x0CF2F032u },
    {   11u, 0x9E3779B1u, 0xE0E99838u },
    {   12u, 0x00000000u, 0xE89B5F9Bu },
    {   12u, 0x9E3779B1u, 0x05A6C4B5u },
    {   13u, 0x00000000u, 0xC537DE02u },
    {   13u, 0x9E3779B1u, 0x1298ADD0u },
    {   14u, 0x00000000u, 0x1208E7E2u },
    {   14u, 0x9E3779B1u, 0x6AF1D1FEu },
    {   15u, 0x00000000u, 0x6B859E14u },
    {   15u, 0x9E3779B1u, 0xAD53090Du },
    {   16u, 0x00000000u, 0x93BA3759u },
    {   16u, 0x9E3779B1u, 0xA94FC1E1u },
    {   17u, 0x00000000u, 0x89FDC23Eu },
    {   17u, 0x9E3779B1u, 0xC9910739u },
    {   18u, 0x00000000u, 0x16B53F56u },
    {   18u, 0x9E3779B1u, 0x5E88116Eu },
    {   19u, 0x00000000u, 0x858FC8EAu },
    {   19u, 0x9E3779B1u, 0x63826A8Fu },
    {   20u, 0x00000000u, 0x38374A35u },
    {   20u, 0x9E3779B1u, 0xC691A55Au },
    {   21u, 0x00000000u, 0x646A88D6u },
    {   21u, 0x9E3779B1u, 0x9668CDABu },
    {   22u, 0x00000000u, 0xB5CCC747u },
    {   22u, 0x9E3779B1u, 0x96F6C6A1u },
    {   23u, 0x00000000u, 0xCC7B3099u },
    {   23u, 0x9E3779B1u, 0x174910FEu },
    {   24u, 0x00000000u, 0xA6276FF0u },
    {   24u, 0x9E3779B1u, 0x7AD49212u },
    {   25u, 0x00000000u, 0x221B959Bu },
    {   25u, 0x9E3779B1u, 0xA1899908u },
    {   26u, 0x00000000u, 0x3F2F5961u },
    {   26u, 0x9E3779B1u, 0x6A936E6Bu },
    {   27u, 0x00000000u, 0x9501BDDDu },
    {   27u, 0x9E3779B1u, 0x61A4E18Bu },
    {   28u, 0x00000000u, 0xDA976167u },
    {   28u, 0x9E3779B1u, 0x261F77CFu },
    {   29u, 0x00000000u, 0x249CC065u },
    {   29u, 0x9E3779B1u, 0xC8EC8904u },
    {   30u, 0x00000000u, 0x36BC3693u },
    {   30u, 0x9E3779B1u, 0xA2800E16u },
    {   31u, 0x00000000u, 0x5F40E562u },
    {   31u, 0x9E3779B1u, 0x5C0C3350u },
    {   32u, 0x00000000u, 0xD89829ECu },
    {   32u, 0x9E3779B1u, 0xA5C44467u },
    {   33u, 0x00000000u, 0x31A427E5u },
    {   33u, 0x9E3779B1u, 0x0DE5B1F9u },
    {   34u, 0x00000000u, 0x80B8584Bu },
    {   34u, 0x9E3779B1u, 0x7FFCCCFEu },
    {   35u, 0x00000000u, 0x7627760Au },
    {   35u, 0x9E3779B1u, 0xD060F480u },
    {   36u, 0x00000000u, 0xED4F560Fu },
    {   36u, 0x9E3779B1u, 0x24321F47u },
    {   37u, 0x00000000u, 0x1241D245u },
    {   37u, 0x9E3779B1u, 0x09EFB299u },
    {   38u, 0x00000000u, 0x5E1CC178u },
    {   38u, 0x9E3779B1u, 0xC1322A61u },
    {   39u, 0x00000000u, 0xBA125684u },
    {   39u, 0x9E3779B1u, 0x2D6246FFu },
    {   40u, 0x00000000u, 0xB22A36ABu },
    {   40u, 0x9E3779B1u, 0xBF491205u },
    {   41u, 0x00000000u, 0x161223AFu },
    {   41u, 0x9E3779B1u, 0x0CB4D70Fu },
    {   42u, 0x00000000u, 0x11586B3Fu },
    {   42u, 0x9E3779B1u, 0x29218DC3u },
    {   43u, 0x00000000u, 0x0A1C7CC9u },
    {   43u, 0x9E3779B1u, 0x4D7337ABu },
    {   44u, 0x00000000u, 0xF0910CD3u },
    {   44u, 0x9E3779B1u, 0xF33E0443u },
    {   45u, 0x00000000u, 0xDD97A408u },
    {   45u, 0x9E3779B1u, 0x8CEC6D9Cu },
    {   46u, 0x00000000u, 0x3A609997u },
    {   46u, 0x9E3779B1u, 0xBD3BA412u },
    {   47u, 0x00000000u, 0xD5E9364Eu },
    {   47u, 0x9E3779B1u, 0xA463C5A4u },
    {   48u, 0x00000000u, 0xBFD05CBDu },
    {   48u, 0x9E3779B1u, 0x0ECCC06Eu },
    {   49u, 0x00000000u, 0xED9B7E81u },
    {   49u, 0x9E3779B1u, 0xA7E98491u },
    {   50u, 0x00000000u, 0xBDF553E2u },
    {   50u, 0x9E3779B1u, 0xA937E7EDu },
    {   51u, 0x00000000u, 0x3CDEED0Fu },
    {   51u, 0x9E3779B1u, 0x9E2ED4B1u },
    {   52u, 0x00000000u, 0x27D50857u },
    {   52u, 0x9E3779B1u, 0x4682E48Bu },
    {   53u, 0x00000000u, 0x62E63D09u },
    {   53u, 0x9E3779B1u, 0x1FF1A182u },
    {   54u, 0x00000000u, 0x30BACCFAu },
    {   54u, 0x9E3779B1u, 0x955B14A7u },
    {   55u, 0x00000000u, 0xEA5D7666u },
    {   55u, 0x9E3779B1u, 0x300DDE4Du },
    {   56u, 0x00000000u, 0xC8D07416u },
    {   56u, 0x9E3779B1u, 0x8952E674u },
    {   57u, 0x00000000u, 0xDF6987F5u },
    {   57u, 0x9E3779B1u, 0x98D545E4u },
    {   58u, 0x00000000u, 0x447DCE59u },
    {   58u, 0x9E3779B1u, 0x1CFC5C29u },
    {   59u, 0x00000000u, 0x4529090Du },
    {   59u, 0x9E3779B1u, 0xE3ACD84Eu },
    {   60u, 0x00000000u, 0xA48E8818u },
    {   60u, 0x9E3779B1u, 0x31CA20F1u },
    {   61u, 0x00000000u, 0xB1C4FBB5u },
    {   61u, 0x9E3779B1u, 0x21749FC9u },
    {   62u, 0x00000000u, 0x84145F58u },
    {   62u, 0x9E3779B1u, 0x0374E669u },
    {   63u, 0x00000000u, 0xF1D48FDBu },
    {   63u, 0x9E3779B1u, 0x956B3D77u },
    {   64u, 0x00000000u, 0x02E95DBBu },
    {   64u, 0x9E3779B1u, 0xCF82F830u },
    {   65u, 0x00000000u, 0x16992B3Du },
    {   65u, 0x9E3779B1u, 0x428EEC5Fu },
    {   66u, 0x00000000u, 0xDEFD9E68u },
    {   66u, 0x9E3779B1u, 0xCED81C34u },
    {   67u, 0x00000000u, 0x9667C027u },
    {   67u, 0x9E3779B1u, 0x56E465E2u },
    {   68u, 0x00000000u, 0xBC606CAAu },
    {   68u, 0x9E3779B1u, 0x3679A1D9u },
    {   69u, 0x00000000u, 0xE7069479u },
    {   69u, 0x9E3779B1u, 0x5D4B3836u },
    {   70u, 0x00000000u, 0xF6EBE19Du },
    {   70u, 0x9E3779B1u, 0x64AD7E72u },
    {   71u, 0x00000000u, 0x7ACA94AAu },
    {   71u, 0x9E3779B1u, 0xA5496174u },
    {   72u, 0x00000000u, 0xB78CCDD2u },
    {   72u, 0x9E3779B1u, 0x6003D7C1u },
    {   73u, 0x00000000u, 0x20957640u },
    {   73u, 0x9E3779B1u, 0x28188381u },
    {   74u, 0x00000000u, 0xC8FEB4EAu },
    {   74u, 0x9E3779B1u, 0x2F41F670u },
    {   75u, 0x00000000u, 0xE1299536u },
    {   75u, 0x9E3779B1u, 0x7EC75712u },
    {   76u, 0x00000000u, 0x120EBEC9u },
    {   76u, 0x9E3779B1u, 0x174F8A9Cu },
    {   77u, 0x00000000u, 0x5F6F8059u },
    {   77u, 0x9E3779B1u, 0xE13D45E7u },
    {   78u, 0x00000000u, 0x2D18D24Cu },
    {   78u, 0x9E3779B1u, 0xFA4D7DC7u },
    {   79u, 0x00000000u, 0x162FFDDCu },
    {   79u, 0x9E3779B1u, 0x17E69038u },
    {   80u, 0x00000000u, 0xB8D7E581u },
    {   80u, 0x9E3779B1u, 0x65D85230u },
    {   81u, 0x00000000u, 0xB13BC253u },
    {   81u, 0x9E3779B1u, 0xAB0B3D44u },
    {   82u, 0x00000000u, 0xA459BF95u },
    {   82u, 0x9E3779B1u, 0xABB92571u },
    {   83u, 0x00000000u, 0x02042BB9u },
    {   83u, 0x9E3779B1u, 0x44F624EDu },
    {   84u, 0x00000000u, 0x5AAC85A4u },
    {   84u, 0x9E3779B1u, 0x890C0E47u },
    {   85u, 0x00000000u, 0x938629B3u },
    {   85u, 0x9E3779B1u, 0x2F13BAC8u },
    {   86u, 0x00000000u, 0xE59B177Eu },
    {   86u, 0x9E3779B1u, 0x24624EDCu },
    {   87u, 0x00000000u, 0xA1874DCCu },
    {   87u, 0x9E3779B1u, 0xD6C644BDu },
    {   88u, 0x00000000u, 0xAB18B20Cu },
    {   88u, 0x9E3779B1u, 0x91094C63u },
    {   89u, 0x00000000u, 0x0BB0289Bu },
    {   89u, 0x9E3779B1u, 0x0A79DCB2u },
    {   90u, 0x00000000u, 0x92AA4D3Cu },
    {   90u, 0x9E3779B1u, 0x5892CA32u },
    {   91u, 0x00000000u, 0x88F76854u },
    {   91u, 0x9E3779B1u, 0x2398BEDAu },
    {   92u, 0x00000000u, 0xD4B4E941u },
    {   92u, 0x9E3779B1u, 0xCBC76755u },
    {   93u, 0x00000000u, 0x44CC8539u },
    {   93u, 0x9E3779B1u, 0x1A2B1728u },
    {   94u, 0x00000000u, 0xEC02C59Fu },
    {   94u, 0x9E3779B1u, 0xA2302AFEu },
    {   95u, 0x00000000u, 0xF081F6AAu },
    {   95u, 0x9E3779B1u, 0x7A8F0916u },
    {   96u, 0x00000000u, 0x4CCF41E0u },
    {   96u, 0x9E3779B1u, 0xCFA0AAECu },
    {   97u, 0x00000000u, 0x914082D0u },
    {   97u, 0x9E3779B1u, 0x9FCA3CB0u },
    {   98u, 0x00000000u, 0x5B313627u },
    {   98u, 0x9E3779B1u, 0x47F152BAu },
    {   99u, 0x00000000u, 0xCC0A29F7u },
    {   99u, 0x9E3779B1u, 0xE3ACED83u },
    {  100u, 0x00000000u, 0x96AD8143u },
    {  100u, 0x9E3779B1u, 0x83D48124u },
    {  101u, 0x00000000u, 0x1B305CDBu },
    {  101u, 0x9E3779B1u, 0x5839BFA6u },
    {  102u, 0x00000000u, 0x3BD5DBACu },
    {  102u, 0x9E3779B1u, 0x78187AB1u },
    {  103u, 0x00000000u, 0x65688574u },
    {  103u, 0x9E3779B1u, 0xEC1A502Cu },
    {  104u, 0x00000000u, 0x78861916u },
    {  104u, 0x9E3779B1u, 0x0F8AC709u },
    {  105u, 0x00000000u, 0x321DE7A2u },
    {  105u, 0x9E3779B1u, 0x2A383E68u },
    {  106u, 0x00000000u, 0x66021791u },
    {  106u, 0x9E3779B1u, 0x48DBC967u },
    {  107u, 0x00000000u, 0x4FBEEA6Du },
    {  107u, 0x9E3779B1u, 0x81DFC749u },
    {  108u, 0x00000000u, 0xBF6E81DAu },
    {  108u, 0x9E3779B1u, 0x6552FB4Cu },
    {  109u, 0x00000000u, 0xAB53BD6Cu },
    {  109u, 0x9E3779B1u, 0x47045E87u },
    {  110u, 0x00000000u, 0x80684E97u },
    {  110u, 0x9E3779B1u, 0xEFD4C511u },
    {  111u, 0x00000000u, 0xB2B2A43Du },
    {  111u, 0x9E3779B1u, 0x7C42B0B3u },
    {  112u, 0x00000000u, 0xBFD42090u },
    {  112u, 0x9E3779B1u, 0xB8FED29Fu },
    {  113u, 0x00000000u, 0x17957E56u },
    {  113u, 0x9E3779B1u, 0x1F838248u },
    {  114u, 0x00000000u, 0x0825AA87u },
    {  114u, 0x9E3779B1u, 0x588CD5CDu },
    {  115u, 0x00000000u, 0x9A1D6116u },
    {  115u, 0x9E3779B1u, 0xB39272D7u },
};

constexpr vec64 xxh64_vectors[] = {
    {    0u, 0x0000000000000000u, 0xEF46DB3751D8E999u },
    {    0u, 0x000000009E3779B1u, 0xAC75FDA2929B17EFu },
    {    1u, 0x0000000000000000u, 0xE934A84ADB052768u },
    {    1u, 0x000000009E3779B1u, 0x5014607643A9B4C3u },
    {    2u, 0x0000000000000000u, 0x5D48CD60A77E23FFu },
    {    2u, 0x000000009E3779B1u, 0x9E93152232D54A39u },
    {    3u, 0x0000000000000000u, 0xFF7E1959CB50794Au },
    {    3u, 0x000000009E3779B1u, 0xAA8584E83660F7D1u },
    {    4u, 0x0000000000000000u, 0x9136A0DCA57457EEu },
    {    4u, 0x000000009E3779B1u, 0xCAAB286BD8E9FDB5u },
    {    5u, 0x0000000000000000u, 0x9B046FB1397F09A5u },
    {    5u, 0x000000009E3779B1u, 0x2AF5249930F984ECu },
    {    6u, 0x0000000000000000u, 0xC72565B7154268A8u },
    {    6u, 0x000000009E3779B1u, 0xCA4C6723580E8EF6u },
    {    7u, 0x0000000000000000u, 0x6C83909A9F01ED25u },
    {    7u, 0x000000009E3779B1u, 0xF98D03B1AD6F9293u },
    {    8u, 0x0000000000000000u, 0xCDBCF538E71D1348u },
    {    8u, 0x000000009E3779B1u, 0xFE0C047A5353CDACu },
    {    9u, 0x0000000000000000u, 0x554B1AE991EDA6B6u },
    {    9u, 0x000000009E3779B1u, 0x7908265248F6D73Fu },
    {   10u, 0x0000000000000000u, 0x5D00E7351392EA84u },
    {   10u, 0x000000009E3779B1u, 0x2A8AE16B86CD2F12u },
    {   11u, 0x0000000000000000u, 0x6345D5746F35DA70u },
    {   11u, 0x000000009E3779B1u, 0xEAA08A8C8BE3CCCFu },
    {   12u, 0x0000000000000000u, 0x0723BF50086EAD9Au },
    {   12u, 0x000000009E3779B1u, 0x8252819F4E506951u },
    {   13u, 0x0000000000000000u, 0xC2E5013E3C40BCF7u },
    {   13u, 0x000000009E3779B1u, 0x4DF437A291CB1039u },
    {   14u, 0x0000000000000000u, 0x8282DCC4994E35C8u },
    {   14u, 0x000000009E3779B1u, 0xC3BD6BF63DEB6DF0u },
    {   15u, 0x0000000000000000u, 0x180719316D622D84u },
    {   15u, 0x000000009E3779B1u, 0xD61105C20E91F99Fu },
    {   16u, 0x0000000000000000u, 0x98C90B57FDFCB55Cu },
    {   16u, 0x000000009E3779B1u, 0xC900AD2D536B607Eu },
    {   17u, 0x0000000000000000u, 0x0D39A2D051A30C2Cu },
    {   17u, 0x000000009E3779B1u, 0x495CD68A647C7A22u },
    {   18u, 0x0000000000000000u, 0x33E84A4333B2B2EBu },
    {   18u, 0x000000009E3779B1u, 0x2325A30CCA1A66DDu },
    {   19u, 0x0000000000000000u, 0xE91C6EF31FC08F82u },
    {   19u, 0x000000009E3779B1u, 0x06809662799B7D6Fu },
    {   20u, 0x0000000000000000u, 0x5F8C68355769439Eu },
    {   20u, 0x000000009E3779B1u, 0x97218696C2D29602u },
    {   21u, 0x0000000000000000u, 0x42B0B8EE353AC461u },
    {   21u, 0x000000009E3779B1u, 0x7FC0BB451B83A633u },
    {   22u, 0x0000000000000000u, 0x65C935C6978098B1u },
    {   22u, 0x000000009E3779B1u, 0xC4A0DD14BF835C13u },
    {   23u, 0x0000000000000000u, 0xD2460ECC840B74DDu },
    {   23u, 0x000000009E3779B1u, 0x4B44E8DE7A396773u },
    {   24u, 0x0000000000000000u, 0xF75A6DEA42DC5BF4u },
    {   24u, 0x000000009E3779B1u, 0x8B7C67EB59778E22u },
    {   25u, 0x0000000000000000u, 0x52FAA43C3F20B994u },
    {   25u, 0x000000009E3779B1u, 0xC4FEC92EAC2C3B8Au },
    {   26u, 0x0000000000000000u, 0x8DB7831EC345F9A3u },
    {   26u, 0x000000009E3779B1u, 0x2C2A80BCAD321466u },
    {   27u, 0x0000000000000000u, 0x88945AA08051FC2Du },
    {   27u, 0x000000009E3779B1u, 0x3401AF8EF28FD410u },
    {   28u, 0x0000000000000000u, 0x64CD9E8C96A9E2DDu },
    {   28u, 0x000000009E3779B1u, 0x8160FB8C20B48287u },
    {   29u, 0x0000000000000000u, 0x8C8F345B634AC2B9u },
    {   29u, 0x000000009E3779B1u, 0x5A327C78E4AD6678u },
    {   30u, 0x0000000000000000u, 0xE2677241D4C46CAFu },
    {   30u, 0x000000009E3779B1u, 0xB1B2B51C93AF4866u },
    {   31u, 0x0000000000000000u, 0x299B39A290E6D783u },
    {   31u, 0x000000009E3779B1u, 0xDA673D5FEB5C1D79u },
    {   32u, 0x0000000000000000u, 0x18B216492BB44B70u },
    {   32u, 0x000000009E3779B1u, 0xB3F33BDF93ADE409u },
    {   33u, 0x0000000000000000u, 0x55C8DC3E578F5B59u },
    {   33u, 0x000000009E3779B1u, 0xE92C292F64BC3071u },
    {   34u, 0x0000000000000000u, 0xD0CE7CBAE371BEB2u },
    {   34u, 0x000000009E3779B1u, 0xC509C460280BBB12u },
    {   35u, 0x0000000000000000u, 0xCB0AA8E2A7A29707u },
    {   35u, 0x000000009E3779B1u, 0xECE3515C236504C7u },
    {   36u, 0x0000000000000000u, 0x0AE67584084DC6A4u },
    {   36u, 0x000000009E3779B1u, 0x57C73F9E3A6FC2C6u },
    {   37u, 0x0000000000000000u, 0x292F5332C92B73CEu },
    {   37u, 0x000000009E3779B1u, 0x7762425B2E89DAD3u },
    {   38u, 0x0000000000000000u, 0x5F398E2A8478BED3u },
    {   38u, 0x000000009E3779B1u, 0xE842D306DC53E621u },
    {   39u, 0x0000000000000000u, 0xB888CAF07592B1B3u },
    {   39u, 0x000000009E3779B1u, 0x602E74D43465AC3Du },
    {   40u, 0x0000000000000000u, 0x23B3810D7D8B1731u },
    {   40u, 0x000000009E3779B1u, 0x5A57F246772BC540u },
    {   41u, 0x0000000000000000u, 0x5342C966C7F3D5F4u },
    {   41u, 0x000000009E3779B1u, 0xB213FFB25FEB1CCDu },
    {   42u, 0x0000000000000000u, 0x77454B74F1DBBF63u },
    {   42u, 0x000000009E3779B1u, 0xAED0CE7EA4202741u },
    {   43u, 0x0000000000000000u, 0x9904C9350F597190u },
    {   43u, 0x000000009E3779B1u, 0xA071200735E57A6Au },
    {   44u, 0x0000000000000000u, 0x24EC2B3D0E442641u },
    {   44u, 0x000000009E3779B1u, 0x831B7452E64EB328u },
    {   45u, 0x0000000000000000u, 0x4EC307EBE0A650A4u },
    {   45u, 0x000000009E3779B1u, 0x66BDA694A1FD3023u },
    {   46u, 0x0000000000000000u, 0x7DD1C5514B2240EFu },
    {   46u, 0x000000009E3779B1u, 0xD238E617805EA9BBu },
    {   47u, 0x0000000000000000u, 0x84A42E167CBD1FEFu },
    {   47u, 0x000000009E3779B1u, 0xA474B31A5132A541u },
    {   48u, 0x0000000000000000u, 0xFD0FEEAC7A939933u },
    {   48u, 0x000000009E3779B1u, 0x6FFE2F43A24C2302u },
    {   49u, 0x0000000000000000u, 0xA91BFF78A7185B66u },
    {   49u, 0x000000009E3779B1u, 0xE858B13C918498ECu },
    {   50u, 0x0000000000000000u, 0xDFFE9F293DDD101Du },
    {   50u, 0x000000009E3779B1u, 0x46521A048859CE7Au },
    {   51u, 0x0000000000000000u, 0x2332585C18D7483Au },
    {   51u, 0x000000009E3779B1u, 0x788846B44826A5D1u },
    {   52u, 0x0000000000000000u, 0x5A0056511F9DD653u },
    {   52u, 0x000000009E3779B1u, 0x1C0081F84C6D32DBu },
    {   53u, 0x0000000000000000u, 0x4426A2F5C8D59469u },
    {   53u, 0x000000009E3779B1u, 0x4270188315F2CC31u },
    {   54u, 0x0000000000000000u, 0x39EAF8893520636Au },
    {   54u, 0x000000009E3779B1u, 0xB6D6E8B68C7EF7A2u },
    {   55u, 0x0000000000000000u, 0xAFEA98CC4E92F7E1u },
    {   55u, 0x000000009E3779B1u, 0xE6D5C88B9135E579u },
    {   56u, 0x0000000000000000u, 0xB7A6414B8D3E2597u },
    {   56u, 0x000000009E3779B1u, 0xB6BD2CBFB6A64017u },
    {   57u, 0x0000000000000000u, 0xF33D64569CD4B1C0u },
    {   57u, 0x000000009E3779B1u, 0xB9B235447E24CA28u },
    {   58u, 0x0000000000000000u, 0x93FA072F0E8FE7CAu },
    {   58u, 0x000000009E3779B1u, 0xB8D0D6211B80EA15u },
    {   59u, 0x0000000000000000u, 0x4F2890576949D6EDu },
    {   59u, 0x000000009E3779B1u, 0x11E586B470B7224Eu },
    {   60u, 0x0000000000000000u, 0x3378C44F459C290Bu },
    {   60u, 0x000000009E3779B1u, 0xA7C18F722446E5C8u },
    {   61u, 0x0000000000000000u, 0x40D486FDDDAD5CB7u },
    {   61u, 0x000000009E3779B1u, 0x980BDE3212E2F025u },
    {   62u, 0x0000000000000000u, 0x21664C60C551CC87u },
    {   62u, 0x000000009E3779B1u, 0x128AC6F704B8A0F7u },
    {   63u, 0x0000000000000000u, 0xA9EFBE0FA0F3F4E7u },
    {   63u, 0x000000009E3779B1u, 0x6C911FADB05B6FC2u },
    {   64u, 0x0000000000000000u, 0xEF558F8ACAC2B5CDu },
    {   64u, 0x000000009E3779B1u, 0xB5EEBA99264CC44Fu },
    {   65u, 0x0000000000000000u, 0xDE0F20DC2631AF7Au },
    {   65u, 0x000000009E3779B1u, 0xD3F6FF3941E310CAu },
    {   66u, 0x0000000000000000u, 0xCF1E52EDE1C505C4u },
    {   66u, 0x000000009E3779B1u, 0xBCE7B3488A29EFB1u },
    {   67u, 0x0000000000000000u, 0x0965DF7219D2E741u },
    {   67u, 0x000000009E3779B1u, 0x1862E884ABDC704Cu },
    {   68u, 0x0000000000000000u, 0x1B8378E923B247A7u },
    {   68u, 0x000000009E3779B1u, 0x25E6F985E36492EFu },
    {   69u, 0x0000000000000000u, 0x43F2B606AD9BA362u },
    {   69u, 0x000000009E3779B1u, 0x81F00574A346F668u },
    {   70u, 0x0000000000000000u, 0xD9C3413222F1DEA4u },
    {   70u, 0x000000009E3779B1u, 0xE01652426AA1774Cu },
    {   71u, 0x0000000000000000u, 0x3D6ECAB2BCFBE3FFu },
    {   71u, 0x000000009E3779B1u, 0xAAB5C313DFAC5A44u },
    {   72u, 0x0000000000000000u, 0xEA8573B60D5A8800u },
    {   72u, 0x000000009E3779B1u, 0xBDEE6BA43947EB30u },
    {   73u, 0x0000000000000000u, 0xE372599E31F8CFFDu },
    {   73u, 0x000000009E3779B1u, 0xF1597384DE9FBD93u },
    {   74u, 0x0000000000000000u, 0x02B86794A00BDBE8u },
    {   74u, 0x000000009E3779B1u, 0x5FB940B90D39A1E6u },
    {   75u, 0x0000000000000000u, 0xCA1A21FB24979810u },
    {   75u, 0x000000009E3779B1u, 0x76214D8E3B169CF5u },
    {   76u, 0x0000000000000000u, 0x60DFF17BEC07766Du },
    {   76u, 0x000000009E3779B1u, 0x9A8133A1DDAF07DBu },
    {   77u, 0x0000000000000000u, 0xF4AF74828DE6862Du },
    {   77u, 0x000000009E3779B1u, 0xD73AEC54F785E384u },
    {   78u, 0x0000000000000000u, 0x828E4C8C5257CC15u },
    {   78u, 0x000000009E3779B1u, 0xD2317E57923E664Cu },
    {   79u, 0x0000000000000000u, 0x91F7F9D6008299A0u },
    {   79u, 0x000000009E3779B1u, 0xE863995625F7AD52u },
    {   80u, 0x0000000000000000u, 0x99BD5D25EB211099u },
    {   80u, 0x000000009E3779B1u, 0x5281D5357D0B8AC4u },
    {   81u, 0x0000000000000000u, 0xCB329A8F102F05BCu },
    {   81u, 0x000000009E3779B1u, 0x7E77B483A01E4AEDu },
    {   82u, 0x0000000000000000u, 0x993A0B6E6D5DF0CBu },
    {   82u, 0x000000009E3779B1u, 0x6DBD4234F13A5DC1u },
    {   83u, 0x0000000000000000u, 0x818F4CEC407D8CF9u },
    {   83u, 0x000000009E3779B1u, 0x0367C5363E62AEC3u },
    {   84u, 0x0000000000000000u, 0xAF57F8CD340FB1B6u },
    {   84u, 0x000000009E3779B1u, 0xC7623B4C4E6844C1u },
    {   85u, 0x0000000000000000u, 0x50789F0F60EBAD3Au },
    {   85u, 0x000000009E3779B1u, 0xB88DA3150AAF44BEu },
    {   86u, 0x0000000000000000u, 0xF3214750C1A6455Eu },
    {   86u, 0x000000009E3779B1u, 0x56C1A7FB96DE0E82u },
    {   87u, 0x0000000000000000u, 0x8DBE8913BE346D20u },
    {   87u, 0x000000009E3779B1u, 0x899E381BD46D1409u },
    {   88u, 0x0000000000000000u, 0xA130814F81F87E43u },
    {   88u, 0x000000009E3779B1u, 0xA3BF96F3D593C759u },
    {   89u, 0x0000000000000000u, 0x0023276D258EDA58u },
    {   89u, 0x000000009E3779B1u, 0x6779DE8C67483444u },
    {   90u, 0x0000000000000000u, 0x0A0571F769B40C93u },
    {   90u, 0x000000009E3779B1u, 0x9C915D4DA4BE05A6u },
    {   91u, 0x0000000000000000u, 0x14A2F5051E8893C8u },
    {   91u, 0x000000009E3779B1u, 0x46F62B0B3D1B66AFu },
    {   92u, 0x0000000000000000u, 0x4FBBE74E062367B0u },
    {   92u, 0x000000009E3779B1u, 0x3190419611427263u },
    {   93u, 0x0000000000000000u, 0x4B375CEF503586FCu },
    {   93u, 0x000000009E3779B1u, 0x821D5495DDDC7C82u },
    {   94u, 0x0000000000000000u, 0x6AD4B96E4286EB70u },
    {   94u, 0x000000009E3779B1u, 0x553DDDB89F3CB523u },
    {   95u, 0x0000000000000000u, 0xFF9F46BDCC644624u },
    {   95u, 0x000000009E3779B1u, 0x72E75A560EF624A3u },
    {   96u, 0x0000000000000000u, 0x105064E743EDD1D9u },
    {   96u, 0x000000009E3779B1u, 0x8FF0B4ABEE6F03CCu },
    {   97u, 0x0000000000000000u, 0x097B16E4E9B0A2E3u },
    {   97u, 0x000000009E3779B1u, 0x9704E5147FDCC4F4u },
    {   98u, 0x0000000000000000u, 0x06662519E8393FF2u },
    {   98u, 0x000000009E3779B1u, 0xB2889379EAD9C5CFu },
    {   99u, 0x0000000000000000u, 0xB51CB5BA2957AB0Bu },
    {   99u, 0x000000009E3779B1u, 0x5C10E54DEC4B0FA9u },
    {  100u, 0x0000000000000000u, 0x4BFE019CD91D9EA4u },
    {  100u, 0x000000009E3779B1u, 0x4853706DC9625CAEu },
    {  101u, 0x0000000000000000u, 0xA51EA2101B2D114Cu },
    {  101u, 0x000000009E3779B1u, 0x6B38A5BBBA1F049Cu },
    {  102u, 0x0000000000000000u, 0x26259500AD475ACFu },
    {  102u, 0x000000009E3779B1u, 0x6CDC4C9926C04118u },
    {  103u, 0x0000000000000000u, 0x74A97F547BB8C845u },
    {  103u, 0x000000009E3779B1u, 0x6A027096FB146E74u },
    {  104u, 0x0000000000000000u, 0x23ED252F132DFA0Fu },
    {  104u, 0x000000009E3779B1u, 0x1683C40FE4F3EC6Eu },
    {  105u, 0x0000000000000000u, 0x1E03CFB52F4E43F2u },
    {  105u, 0x000000009E3779B1u, 0x8399AE3FAC887392u },
    {  106u, 0x0000000000000000u, 0xB3C7FBBF1E2E9E9Fu },
    {  106u, 0x000000009E3779B1u, 0x205158F71E4AD7DAu },
    {  107u, 0x0000000000000000u, 0xC85AF48447573841u },
    {  107u, 0x000000009E3779B1u, 0x42BE4CF154BDCB6Fu },
    {  108u, 0x0000000000000000u, 0x395022FA4E7C679Du },
    {  108u, 0x000000009E3779B1u, 0x76577A7F0C793723u },
    {  109u, 0x0000000000000000u, 0x931CE3A962248CB1u },
    {  109u, 0x000000009E3779B1u, 0x3F3AFDE1CBFB2D78u },
    {  110u, 0x0000000000000000u, 0x46A7A7126CE7D4E6u },
    {  110u, 0x000000009E3779B1u, 0x6CC264DBC30D3EDCu },
    {  111u, 0x0000000000000000u, 0xB7AFFC461C9ECE18u },
    {  111u, 0x000000009E3779B1u, 0x7D83E9E9160317ECu },
    {  112u, 0x0000000000000000u, 0xE5752EA6E2B34417u },
    {  112u, 0x000000009E3779B1u, 0xEECB8B56A5BD00DBu },
    {  113u, 0x0000000000000000u, 0xFDA17FD84B50AD25u },
    {  113u, 0x000000009E3779B1u, 0x47E8C074796FD671u },
    {  114u, 0x0000000000000000u, 0x8751DAC0314381B6u },
    {  114u, 0x000000009E3779B1u, 0xB880FBDD4FB6B274u },
    {  115u, 0x0000000000000000u, 0xF7CB3E3DFCE93157u },
    {  115u, 0x000000009E3779B1u, 0x55033F2004F0E78Cu },
};

/* Same pseudorandom buffer the upstream vectors were generated against. */
constexpr size_t test_buffer_size = 4096 + 64 + 1;
void fill_test_buffer(byte* buffer) noexcept {
    core::u64 byte_gen = 2654435761ull;
    for (size_t i = 0; i < test_buffer_size; ++i) {
        buffer[i] = (byte)(byte_gen >> 56);
        byte_gen *= 11400714785074694797ull;
    }
}

} // namespace

using core::xxh32;
using core::xxh64;
using core::xxhash32;
using core::xxhash64;

TEST_CASE("xxhash32") {
    byte buf[test_buffer_size];
    fill_test_buffer(buf);

    SECTION("known answer vectors, one-shot") {
        for (const vec32& v : xxh32_vectors)
            CHECK(xxh32(buf, v.len, v.seed) == v.result);
    }

    SECTION("known answer vectors, streamed one byte at a time") {
        xxhash32 st;
        for (const vec32& v : xxh32_vectors) {
            st.reset(v.seed);
            for (core::u32 i = 0; i < v.len; ++i)
                st.update(&buf[i], 1);
            REQUIRE(st.length() == v.len);
            CHECK(st.digest() == v.result);
        }
    }

    SECTION("streamed in arbitrary chunks matches one-shot") {
        constexpr size_t size = test_buffer_size;
        const core::u32 seeds[] = {0u, 0x9E3779B1u, 0xDEADBEEFu};
        const size_t chunks[] = {1, 2, 3, 7, 15, 16, 17, 31, 100, 1000};
        for (core::u32 seed : seeds) {
            const core::u32 expected = xxh32(buf, size, seed);
            for (size_t chunk : chunks) {
                xxhash32 st{seed};
                for (size_t off = 0; off < size; off += chunk) {
                    size_t n = size - off;
                    if (n > chunk)
                        n = chunk;
                    st.update(buf + off, n);
                }
                REQUIRE(st.length() == size);
                CHECK(st.digest() == expected);
            }
        }
    }

    SECTION("digest is non-destructive and state can be continued") {
        xxhash32 st{7};
        st.update(buf, 100);
        const core::u32 h1 = st.digest();
        CHECK(st.digest() == h1); // repeated digest is stable
        st.update(buf + 100, 100);
        CHECK(st.digest() == xxh32(&buf[0], 200, 7));

        st.reset(7);
        CHECK(st.digest() == xxh32(nullptr, 0, 7));
        st.update(buf, 200);
        CHECK(st.digest() == xxh32(buf, 200, 7));
    }

    SECTION("copying a state preserves the stream") {
        xxhash32 a{123};
        a.update(buf, 77);
        xxhash32 b = a;
        CHECK(b.digest() == a.digest());
        a.update(buf + 77, 50);
        b.update(buf + 77, 50);
        CHECK(b.digest() == a.digest());
        CHECK(a.digest() == xxh32(buf, 127, 123));
    }

    SECTION("empty and zero-size inputs") {
        CHECK(xxh32(nullptr, 0) == xxh32_vectors[0].result); // seed 0, len 0
        xxhash32 st;
        st.update(nullptr, 0);
        st.update(buf, 0);
        CHECK(st.digest() == xxh32_vectors[0].result);
    }

    SECTION("different seeds give different hashes") {
        CHECK(xxh32(buf, 1000, 1) != xxh32(buf, 1000, 2));
    }
}

TEST_CASE("xxhash64") {
    byte buf[test_buffer_size];
    fill_test_buffer(buf);

    SECTION("known answer vectors, one-shot") {
        for (const vec64& v : xxh64_vectors)
            CHECK(xxh64(buf, v.len, v.seed) == v.result);
    }

    SECTION("known answer vectors, streamed one byte at a time") {
        xxhash64 st;
        for (const vec64& v : xxh64_vectors) {
            st.reset(v.seed);
            for (core::u32 i = 0; i < v.len; ++i)
                st.update(&buf[i], 1);
            REQUIRE(st.length() == v.len);
            CHECK(st.digest() == v.result);
        }
    }

    SECTION("streamed in arbitrary chunks matches one-shot") {
        constexpr size_t size = test_buffer_size;
        const core::u64 seeds[] = {0ull, 0x9E3779B1ull, 0x1234567890ABCDEFull};
        const size_t chunks[] = {1, 2, 3, 7, 31, 32, 33, 63, 64, 65, 100, 1000};
        for (core::u64 seed : seeds) {
            const core::u64 expected = xxh64(buf, size, seed);
            for (size_t chunk : chunks) {
                xxhash64 st{seed};
                for (size_t off = 0; off < size; off += chunk) {
                    size_t n = size - off;
                    if (n > chunk)
                        n = chunk;
                    st.update(buf + off, n);
                }
                REQUIRE(st.length() == size);
                CHECK(st.digest() == expected);
            }
        }
    }

    SECTION("digest is non-destructive and state can be continued") {
        xxhash64 st{7};
        st.update(buf, 100);
        const core::u64 h1 = st.digest();
        CHECK(st.digest() == h1); // repeated digest is stable
        st.update(buf + 100, 100);
        CHECK(st.digest() == xxh64(&buf[0], 200, 7));

        st.reset(7);
        CHECK(st.digest() == xxh64(nullptr, 0, 7));
        st.update(buf, 200);
        CHECK(st.digest() == xxh64(buf, 200, 7));
    }

    SECTION("copying a state preserves the stream") {
        xxhash64 a{123};
        a.update(buf, 77);
        xxhash64 b = a;
        CHECK(b.digest() == a.digest());
        a.update(buf + 77, 50);
        b.update(buf + 77, 50);
        CHECK(b.digest() == a.digest());
        CHECK(a.digest() == xxh64(buf, 127, 123));
    }

    SECTION("empty and zero-size inputs") {
        CHECK(xxh64(nullptr, 0) == xxh64_vectors[0].result); // seed 0, len 0
        xxhash64 st;
        st.update(nullptr, 0);
        st.update(buf, 0);
        CHECK(st.digest() == xxh64_vectors[0].result);
    }

    SECTION("different seeds give different hashes") {
        CHECK(xxh64(buf, 1000, 1) != xxh64(buf, 1000, 2));
    }
}
