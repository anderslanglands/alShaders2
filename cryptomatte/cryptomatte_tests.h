/*

Testing documentation:

The supplied python unit tests of this repo should run these tests.

Tests run on node_init, if:
    * Cryptomatte node has boolean user data, "run_unit_tests"
    * run_unit_tests is set to true

For example:
    declare run_unit_tests constant BOOL
    run_unit_tests true

The unit tests will run as part of node init, when CryptomatteData is constructed.

In this test suite, an assertion failure results in an AiMsgError.

*/

#define CRYPTO_TEST_FLAG "run_unit_tests"

///////////////////////////////////////////////
//
//    Namr parsing tests
//
///////////////////////////////////////////////

// todo(jonah): the excessive "(char)"'s are to silence a warning about conversion. '
static const char test_utf8_pabhnha[] = {
    (char)0xd1, (char)0x80, (char)0xd0, (char)0xb0, (char)0xd0, (char)0xb2, (char)0xd0, (char)0xbd,
    (char)0xd0, (char)0xb8, (char)0xd0, (char)0xbd, (char)0xd0, (char)0xb0, '\0'}; //равнина
static const char test_utf8_madchen[] = {'m', (char)0xc3, (char)0xa4, 'd', 'c',
                                         'h', 'e',        'n',        '\0'}; // mädchen

namespace NameParsingTests {
inline void assert_clean_names(const char* msg, const char* obj_name_in, bool strip_obj_ns,
                               const char* obj_correct, const char* nsp_correct) {
    char obj_name_out[MAX_STRING_LENGTH] = "", nsp_name_out[MAX_STRING_LENGTH] = "";
    get_clean_object_name(obj_name_in, obj_name_out, nsp_name_out, strip_obj_ns);
    /*
        null "correct" names mean just check there was no crash and the result is a string
        AiMsgInfo reporting makes sure compiller can't remove this
    */
    if (obj_correct == NULL || nsp_correct == NULL)
        AiMsgDebug("Success (%s did not crash) - %lu %lu", msg, strlen(obj_name_out),
                   strlen(nsp_name_out));
    else if (strncmp(obj_name_out, obj_correct, -1) != 0)
        AiMsgError("get_clean_object: OBJECT name mismatch: ((%s)) Expected %s, was %s", msg,
                   obj_correct, obj_name_out);
    else if (strncmp(nsp_name_out, nsp_correct, -1) != 0)
        AiMsgError("get_clean_object: NAMESPACE mismatch: ((%s)) Expected %s, was %s", msg,
                   nsp_correct, nsp_name_out);
    AiMsgDebug("Test completed (%s)", msg);
}

inline void assert_name_doesnt_crash(const char* msg, const char* obj_name_in, bool strip_obj_ns) {
    assert_clean_names(msg, obj_name_in, strip_obj_ns, NULL, NULL);
}

inline void assert_sitoa_inst_handling(const char* msg, const char* obj_name_in,
                                       const char* obj_correct, bool handled_correct) {
    char obj_name_out[MAX_STRING_LENGTH] = "";
    bool handled = sitoa_pointcloud_instance_handling(obj_name_in, obj_name_out);

    if (handled_correct != handled)
        AiMsgError("Should have been handled, wasn't. (%s)", msg);

    if (obj_correct == NULL)
        AiMsgDebug("Success - %lu", strlen(obj_name_out));
    else if (handled && strncmp(obj_name_out, obj_correct, -1) != 0)
        AiMsgError("sitoa pointcloud handling: ((%s)) Expected %s, was %s", msg, obj_correct,
                   obj_name_out);
    AiMsgDebug("Test ran: %s", msg);
}

inline void assert_mtoa_strip(const char* msg, const char* obj_name_in, const char* obj_correct) {
    char obj_name_out[MAX_STRING_LENGTH] = "";
    mtoa_strip_namespaces(obj_name_in, obj_name_out);

    if (obj_correct == NULL)
        AiMsgDebug("Success - %lu", strlen(obj_name_out));
    else if (strncmp(obj_name_out, obj_correct, -1) != 0)
        AiMsgError("MtoA ns stripping: ((%s)) Expected %s, was %s", msg, obj_correct, obj_name_out);
    AiMsgDebug("Test ran: %s", msg);
}

inline void mtoa_parsing() {
    assert_clean_names("mtoa-1", "object", true, "object", "default");
    assert_clean_names("mtoa-2", "ns1:obj1", true, "obj1", "ns1");
    assert_clean_names("mtoa-3", "ns1:ns2:obj1", true, "ns2:obj1", "ns1");
    assert_clean_names("mtoa-4", "ns1:obj1", false, "ns1:obj1", "ns1");
    // test gaetan's smarter maya namespace handling
    assert_clean_names("mtoa-5", "namespace:object|ns2:obj2", true, "object|obj2", "namespace");
}

inline void mtoa_strip() {
    assert_mtoa_strip("mtoa-1b", "object", "object");
    assert_mtoa_strip("mtoa-2b", "namespace:object", "object");
    assert_mtoa_strip("mtoa-3b", "ns1:obj1|ns2:obj2", "obj1|obj2");
    assert_mtoa_strip("mtoa-4b", "obj1|ns2:obj2|aaa:bbb", "obj1|obj2|bbb");
    assert_mtoa_strip("mtoa-5b", "obj1|aaa:bbb|ccc:ddd|eee", "obj1|bbb|ddd|eee");
    assert_mtoa_strip("mtoa-6b", "obj1|aaa|ccc:ddd|eee:fff", "obj1|aaa|ddd|fff");
}

inline void sitoa_parsing() {
    assert_clean_names("sitoa-1", "object.SItoA.1002", true, "object", "default");
    assert_clean_names("sitoa-2", "model.object.SItoA.1002", true, "object", "model");
    assert_clean_names("sitoa-3", "model.object.SItoA.1002", false, "model.object", "model");

    const char* sitoa_inst_schema = "mdl.icecloud.SItoA.Instance.<frame>.<id> mstr.SItoA.1001";
    g_pointcloud_instance_verbosity = 0;
    assert_clean_names("sitoa-4", sitoa_inst_schema, true, "icecloud", "mdl");
    assert_sitoa_inst_handling("sitoa-4b", sitoa_inst_schema, "", false);
    g_pointcloud_instance_verbosity = 1;
    assert_clean_names("sitoa-5", sitoa_inst_schema, true, "mstr", "mdl");
    assert_sitoa_inst_handling("sitoa-5b", sitoa_inst_schema, "mstr", true);
    g_pointcloud_instance_verbosity = 2;
    assert_clean_names("sitoa-6", sitoa_inst_schema, true, "mstr.<id>", "mdl");
    assert_sitoa_inst_handling("sitoa-6b", sitoa_inst_schema, "mstr.<id>", true);

    const char* sitoa_inst = "mdl.icecloud.SItoA.Instance.1001.47 master.SItoA.1001";
    g_pointcloud_instance_verbosity = 0;
    assert_clean_names("sitoa-7", sitoa_inst, true, "icecloud", "mdl");
    assert_sitoa_inst_handling("sitoa-7b", sitoa_inst_schema, "", false);

    g_pointcloud_instance_verbosity = 1;
    assert_clean_names("sitoa-8", sitoa_inst, true, "master", "mdl");
    assert_sitoa_inst_handling("sitoa-8b", sitoa_inst, "master", true);

    g_pointcloud_instance_verbosity = 2;
    assert_clean_names("sitoa-9", sitoa_inst, true, "master.47", "mdl");
    assert_sitoa_inst_handling("sitoa-9b", sitoa_inst, "master.47", true);

    g_pointcloud_instance_verbosity = 0;
    assert_clean_names("sitoa-10", sitoa_inst, false, "mdl.icecloud", "mdl");
    assert_sitoa_inst_handling("sitoa-10b", sitoa_inst, "", false);

    g_pointcloud_instance_verbosity = 1;
    assert_clean_names("sitoa-11", sitoa_inst, false, "master", "mdl");
    assert_sitoa_inst_handling("sitoa-11b", sitoa_inst, "master", true);

    g_pointcloud_instance_verbosity = 2;
    assert_clean_names("sitoa-12", sitoa_inst, false, "master.47", "mdl");
    assert_sitoa_inst_handling("sitoa-12b", sitoa_inst, "master.47", true);

    g_pointcloud_instance_verbosity = 0;
    const char* sitoa_inst_no_mdl = "icecloud.SItoA.Instance.1001.47 master.SItoA.1001";
    assert_clean_names("sitoa-13", sitoa_inst_no_mdl, false, "icecloud", "default");
    assert_sitoa_inst_handling("sitoa-13b", sitoa_inst_no_mdl, "", false);

    // reset
    g_pointcloud_instance_verbosity = CRYPTO_ICEPCLOUDVERB_DEFAULT;
}

inline void c4dtoa_parsing() {
    // c4d style
    assert_clean_names("c4d-1", "c4d|hi|er|arch|chy", true, "chy", "hi|er|arch");
    assert_clean_names("c4d-2", "c4d|group|object", true, "object", "group");
    assert_clean_names("c4d-3", "c4d|object", true, "object", "default");
    assert_clean_names("c4d-4", "c4d|hi|er|arch|chy", false, "hi|er|arch|chy", "hi|er|arch");
}

inline std::string long_string(std::string input, int doublings) {
    std::string result(input);
    for (uint32_t i = 0; i < 10; i++) // lenght doubles each time
        result += result;
    return result;
}

inline void crazy_maya_parsing() {
    std::string looong =
        std::string(long_string("namespace", 10) + ":" + long_string("object", 10));
    std::string weird = long_string("ns:obj", 10);

    assert_name_doesnt_crash("mtoa-6", looong.c_str(), true);
    assert_name_doesnt_crash("mtoa-7", weird.c_str(), true);
}

inline void crazy_sitoa_parsing() {
    const char* crashed_parsing = "mdl.icecloud.SItoA.Instance.<frame> <id> mstr.SItoA.1001";
    g_pointcloud_instance_verbosity = 2;
    assert_name_doesnt_crash("sitoa-ugly-1", crashed_parsing, false);
    assert_sitoa_inst_handling("sitoa-ugly-14b", crashed_parsing, NULL, false);

    std::string looong = long_string("model", 10) + "." + long_string("object", 10) + ".SItoA.1001";
    std::string weird = long_string("mdl.obj", 10);
    std::string longInst = "mdl." + long_string("icecloud", 10) + ".SItoA.Instance.1001." + "400" +
                           " " +                                          // instance number + space
                           long_string("masterName", 10) + ".SItoA.1001"; // master name

    assert_name_doesnt_crash("sitoa-ugly-2", looong.c_str(), false);
    assert_sitoa_inst_handling("sitoa-ugly-2b", looong.c_str(), NULL, false);
    assert_name_doesnt_crash("sitoa-ugly-3", weird.c_str(), false);
    assert_sitoa_inst_handling("sitoa-ugly-3b", weird.c_str(), NULL, false);
    assert_name_doesnt_crash("sitoa-ugly-4", longInst.c_str(), false);
    assert_sitoa_inst_handling("sitoa-ugly-4b", longInst.c_str(), NULL, false);
}

inline void malformed_name_parsing() {
    // just want to know there's no crashing
    assert_name_doesnt_crash("malformed-0", "", true);
    assert_name_doesnt_crash("malformed-1", ":obj", true);
    assert_name_doesnt_crash("malformed-2", "|", true);
    assert_name_doesnt_crash("malformed-3", "|obj", true);
    assert_name_doesnt_crash("malformed-4", "par", true);
    assert_name_doesnt_crash("malformed-5", "ns:", true);
    assert_name_doesnt_crash("malformed-6", ".", true);
    assert_name_doesnt_crash("malformed-7", "c4d|", true);
    assert_name_doesnt_crash("malformed-8", "SItoA", true);
    assert_name_doesnt_crash("malformed-9", ".SItoA", true);
}

inline void utf8_parsing() {
    // mädchen:равнина -> равнина, mädchen
    std::string m_p = std::string(test_utf8_madchen) + ":" + std::string(test_utf8_pabhnha);
    assert_clean_names("utf8-mayastyle-1", m_p.c_str(), true, test_utf8_pabhnha, test_utf8_madchen);
    // mädchen:равнина|равнина -> равнина|равнина, mädchen
    assert_clean_names(
        "utf8-mayastyle-2", (m_p + "|" + std::string(test_utf8_pabhnha)).c_str(), true,
        (std::string(test_utf8_pabhnha) + "|" + std::string(test_utf8_pabhnha)).c_str(),
        test_utf8_madchen);
}

inline void run() {
    mtoa_parsing();
    mtoa_strip();
    sitoa_parsing();
    c4dtoa_parsing();
    crazy_maya_parsing();
    crazy_sitoa_parsing();
    malformed_name_parsing();
    utf8_parsing();
    AiMsgInfo("Cryptomatte unit tests: Name parsing checks complete");
}
} // namespace NameParsingTests

namespace MaterialNameTests {

inline void assert_material_name(const char* msg, const char* mat_full_name, bool strip_ns,
                                 const char* mat_correct) {
    char mat_name_out[MAX_STRING_LENGTH] = "";
    get_clean_material_name(mat_full_name, mat_name_out, strip_ns);

    if (mat_correct == NULL)
        AiMsgDebug("Success (%s did not crash) - %lu", msg, strlen(mat_name_out));
    else if (strncmp(mat_correct, mat_name_out, -1) != 0)
        AiMsgError("get_clean_material: Mismatch: ((%s)) Expected %s, was %s", msg, mat_correct,
                   mat_name_out);
}

inline void test_c4d_names() {
    assert_material_name("c4d-m-1", "c4d|mat_name|root_node_name", true, "mat_name");
    // todo(jonah): is this correct?
    assert_material_name("c4d-m-2", "c4d|mat_name|root_node_name", false, "mat_name");
}
inline void test_mtoa_names() {
    assert_material_name("mtoa-m-1", "namespace:my_material_sg", true, "my_material_sg");
    assert_material_name("mtoa-m-2", "namespace:my_material_sg", false, "namespace:my_material_sg");
    assert_material_name("mtoa-m-3", "my_material_sg", true, "my_material_sg");
    assert_material_name("mtoa-m-4", "", true, NULL);
    assert_material_name("mtoa-m-5", ":", true, NULL);
    assert_material_name("mtoa-m-6", ":my_material_sg", true, "my_material_sg");
}
inline void test_sitoa_names() {
    const char sitoa_mat[] = "Sources.Materials.myLibrary_ref_library.myMaterialName."
                             "Standard_Mattes.uBasic.SItoA.25000...";
    assert_material_name("sitoa-m-1", sitoa_mat, true, "myMaterialName");
    assert_material_name("sitoa-m-2", sitoa_mat, false, "myLibrary_ref_library.myMaterialName");
}

inline void run() {
    test_c4d_names();
    test_mtoa_names();
    test_sitoa_names();
}
} // namespace MaterialNameTests

namespace HashingTests {
inline void assert_hash_to_float(const char* name, float expected_hash) {
    uint32_t m3hash = 0;
    MurmurHash3_x86_32(name, (uint32_t)strlen(name), 0, &m3hash);
    float hash = hash_to_float(m3hash);
    if (expected_hash != hash)
        AiMsgError("(f) hash mismatch: (%s) Expected %g, was %g", name, expected_hash, hash);
}

inline void hash_ascii_names() {
    assert_hash_to_float("hello", 6.0705627102400005616e-17f);
    assert_hash_to_float("cube", -4.08461912519e+15f);
    assert_hash_to_float("sphere", 2.79018604383e+15f);
    assert_hash_to_float("plane", 3.66557617593e-11f);
}

inline void hash_utf8_names() {
    assert_hash_to_float(test_utf8_pabhnha, -1.3192631212399999468e-25f);
    // assert_hash_to_float("равнина", -1.3192631212399999468e-25f);

    assert_hash_to_float(test_utf8_madchen, 6.2361298211599995797e+25f);
    // assert_hash_to_float("mädchen", 6.2361298211599995797e+25f);
}

inline void run() {
    hash_ascii_names();
    hash_utf8_names();
}
} // namespace HashingTests

inline void run_all_unit_tests(AtNode* node) {
    if (node && AiNodeLookUpUserParameter(node, CRYPTO_TEST_FLAG) &&
        AiNodeGetBool(node, CRYPTO_TEST_FLAG)) {
        AiMsgWarning("Cryptomatte unit tests: Running");
        NameParsingTests::run();
        HashingTests::run();
        MaterialNameTests::run();
        AiMsgWarning("Cryptomatte unit tests: Complete");
    }
}


