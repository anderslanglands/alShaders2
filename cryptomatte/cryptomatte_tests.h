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

#define CRYPTOMATTE_UNIT_TEST_FLAG "run_unit_tests"

///////////////////////////////////////////////
//
//    Namr parsing tests
//
///////////////////////////////////////////////

namespace NameParsingTests {
    void assert_clean_names(std::string msg, std::string obj_name_in, bool strip_obj_ns,
                             const char *obj_correct, const char *nsp_correct) {
        char obj_name_out[MAX_STRING_LENGTH] = "", nsp_name_out[MAX_STRING_LENGTH] = "";
        get_clean_object_name(obj_name_in.c_str(), obj_name_out, nsp_name_out, strip_obj_ns);

        /*
            null "correct" names mean just check there was (no crash) and the result is a string 
            AiMsgInfo reporting makes sure compiller can't remove this
        */
        if (obj_correct == NULL || nsp_correct == NULL)
            AiMsgDebug("Success (%s did not crash) - %lu %lu", 
                       msg, strlen(obj_name_out), strlen(nsp_name_out));
        else if (strncmp(obj_name_out, obj_correct, -1) != 0)
            AiMsgError("get_clean_object: OBJECT name mismatch: ((%s)) Expected %s, was %s", 
                       msg.c_str(), obj_correct, obj_name_out);
        else if (strncmp(nsp_name_out, nsp_correct, -1) != 0)
            AiMsgError("get_clean_object: NAMESPACE mismatch: ((%s)) Expected %s, was %s", 
                       msg.c_str(), nsp_correct, nsp_name_out);
        AiMsgDebug("Test completed (%s)", msg);
    }

    void assert_sitoa_inst_handling(std::string msg, std::string obj_name_in, 
                             const char *obj_correct, bool handled_correct) {
        char obj_name_out[MAX_STRING_LENGTH] = "";
        bool handled = sitoa_pointcloud_instance_handling(obj_name_in.c_str(), obj_name_out);

        if (handled_correct != handled)
            AiMsgError("Should have been handled, wasn't. (%s)", msg.c_str());

        if (obj_correct == NULL)
            AiMsgDebug("Success - %lu", strlen(obj_name_out));
        else if (handled && strncmp(obj_name_out, obj_correct, -1) != 0)
            AiMsgError("sitoa pointcloud handling: ((%s)) Expected %s, was %s", 
                       msg.c_str(), obj_correct, obj_name_out);
        AiMsgDebug("Test ran: %s", msg);
    }

    void assert_mtoa_strip(std::string msg, std::string obj_name_in, const char *obj_correct) {
        char obj_name_out[MAX_STRING_LENGTH] = "";
        mtoa_strip_namespaces(obj_name_in.c_str(), obj_name_out);

        if (obj_correct == NULL)
            AiMsgDebug("Success - %lu", strlen(obj_name_out));
        else if (strncmp(obj_name_out, obj_correct, -1) != 0)
            AiMsgError("MtoA ns stripping: ((%s)) Expected %s, was %s", 
                       msg.c_str(), obj_correct, obj_name_out);
        AiMsgDebug("Test ran: %s", msg);
    }

    void mtoa_parsing() {    
        assert_clean_names("mtoa-1", "object", true, 
                           "object", "default");
        assert_clean_names("mtoa-2", "ns1:obj1", true, 
                           "obj1", "ns1");
        assert_clean_names("mtoa-3", "ns1:ns2:obj1", true, 
                           "ns2:obj1", "ns1");
        assert_clean_names("mtoa-4", "ns1:obj1", false, 
                           "ns1:obj1", "ns1");
        // test gaetan's smarter maya namespace handling
        assert_clean_names("mtoa-5", "namespace:object|ns2:obj2", true, 
                           "object|obj2", "namespace");
    }

    void mtoa_strip() {    
        assert_mtoa_strip("mtoa-1b", "object", "object");
        assert_mtoa_strip("mtoa-2b", "namespace:object", "object");
        assert_mtoa_strip("mtoa-3b", "ns1:obj1|ns2:obj2", "obj1|obj2");
        assert_mtoa_strip("mtoa-4b", "ns1:obj1|ns2:obj2|aaa:bbb", "obj1|obj2|bbb");
        assert_mtoa_strip("mtoa-5b", "ns1:obj1|aaa:bbb|ccc:ddd|eee", "obj1|bbb|ddd|eee");
        assert_mtoa_strip("mtoa-6b", "ns1:obj1|aaa|ccc:ddd|eee:fff", "obj1|aaa|ddd|fff");
    }

    void sitoa_parsing() {    
        assert_clean_names("sitoa-1", "object.SItoA.1002", true, 
                           "object", "default");
        assert_clean_names("sitoa-2", "model.object.SItoA.1002", true, 
                           "object", "model");
        assert_clean_names("sitoa-3", "model.object.SItoA.1002", false, 
                           "model.object", "model");

        const char *sitoa_inst_schema = "mdl.icecloud.SItoA.Instance.<frame>.<id> mstr.SItoA.1001";
        g_pointcloud_instance_verbosity = 0;
        assert_clean_names("sitoa-4", sitoa_inst_schema, true, 
                           "icecloud", "mdl");
        assert_sitoa_inst_handling("sitoa-4b", sitoa_inst_schema, "", false);
        g_pointcloud_instance_verbosity = 1;
        assert_clean_names("sitoa-5", sitoa_inst_schema, true, 
                           "mstr", "mdl");
        assert_sitoa_inst_handling("sitoa-5b", sitoa_inst_schema, "mstr", true);
        g_pointcloud_instance_verbosity = 2;
        assert_clean_names("sitoa-6", sitoa_inst_schema, true, 
                           "mstr.<id>", "mdl");
        assert_sitoa_inst_handling("sitoa-6b", sitoa_inst_schema, "mstr.<id>", true);

        const char *sitoa_inst = "mdl.icecloud.SItoA.Instance.1001.47 master.SItoA.1001";
        g_pointcloud_instance_verbosity = 0;
        assert_clean_names("sitoa-7", sitoa_inst, true, 
                           "icecloud", "mdl");
        assert_sitoa_inst_handling("sitoa-7b", sitoa_inst_schema, "", false);

        g_pointcloud_instance_verbosity = 1;
        assert_clean_names("sitoa-8", sitoa_inst, true, 
                           "master", "mdl");
        assert_sitoa_inst_handling("sitoa-8b", sitoa_inst, "master", true);

        g_pointcloud_instance_verbosity = 2;
        assert_clean_names("sitoa-9", sitoa_inst, true, 
                           "master.47", "mdl");
        assert_sitoa_inst_handling("sitoa-9b", sitoa_inst, "master.47", true);

        g_pointcloud_instance_verbosity = 0;
        assert_clean_names("sitoa-10", sitoa_inst, false, 
                           "mdl.icecloud", "mdl");
        assert_sitoa_inst_handling("sitoa-10b", sitoa_inst, "", false);

        g_pointcloud_instance_verbosity = 1;
        assert_clean_names("sitoa-11", sitoa_inst, false, 
                           "master", "mdl");
        assert_sitoa_inst_handling("sitoa-11b", sitoa_inst, "master", true);

        g_pointcloud_instance_verbosity = 2;
        assert_clean_names("sitoa-12", sitoa_inst, false, 
                           "master.47", "mdl");
        assert_sitoa_inst_handling("sitoa-12b", sitoa_inst, "master.47", true);

        g_pointcloud_instance_verbosity = 0;
        const char *sitoa_inst_no_mdl = "icecloud.SItoA.Instance.1001.47 master.SItoA.1001";
        assert_clean_names("sitoa-13", sitoa_inst_no_mdl, false, 
                           "icecloud", "default");
        assert_sitoa_inst_handling("sitoa-13b", sitoa_inst_no_mdl, "", false);

        // reset
        g_pointcloud_instance_verbosity = CRYPTO_ICEPCLOUDVERB_DEFAULT;
    }

    void c4dtoa_parsing() {    
        // c4d style
        assert_clean_names("c4d-1", "c4d|hi|er|arch|chy", true, 
                           "chy", "hi|er|arch");
        assert_clean_names("c4d-2", "c4d|group|object", true, 
                           "object", "group");
        assert_clean_names("c4d-3", "c4d|object", true, 
                           "object", "default");    
        assert_clean_names("c4d-4", "c4d|hi|er|arch|chy", false, 
                           "hi|er|arch|chy", "hi|er|arch");
    }

    std::string long_string(std::string input, int doublings) {
        std::string result(input);
        for (uint32_t i=0; i<10; i++) // lenght doubles each time
            result += result;
        return result;
    }

    void crazy_maya_parsing() {    
        std::string looong = std::string(
            long_string("namespace", 10) 
            + ":" 
            + long_string("object", 10)
        );
        std::string weird = long_string("ns:obj", 10);

        assert_clean_names("mtoa-6", looong, true, NULL, NULL);
        assert_clean_names("mtoa-7", weird, true, NULL, NULL);
    }

    void crazy_sitoa_parsing() {
        const char *crashed_parsing = "mdl.icecloud.SItoA.Instance.<frame> <id> mstr.SItoA.1001";
        g_pointcloud_instance_verbosity = 2;
        assert_clean_names("sitoa-ugly-1", crashed_parsing, false, NULL, NULL);
        assert_sitoa_inst_handling("sitoa-ugly-14b", crashed_parsing, NULL, false);

        std::string looong = 
            long_string("model", 10) + "." + long_string("object", 10) + ".SItoA.1001";
        std::string weird = long_string("mdl.obj", 10);
        std::string longInst = 
            "mdl." + long_string("icecloud", 10) + ".SItoA.Instance.1001." + 
            "400" + " " + // instance number + space
            long_string("masterName", 10) + ".SItoA.1001"; // master name

        assert_clean_names("sitoa-ugly-2", looong, false, NULL, NULL);
        assert_sitoa_inst_handling("sitoa-ugly-2b", looong, NULL, false);
        assert_clean_names("sitoa-ugly-3", weird, false, NULL, NULL);
        assert_sitoa_inst_handling("sitoa-ugly-3b", weird, NULL, false);
        assert_clean_names("sitoa-ugly-4", longInst, false, NULL, NULL);
        assert_sitoa_inst_handling("sitoa-ugly-4b", longInst, NULL, false);
    }

    void run() {
        mtoa_parsing();
        mtoa_strip();
        sitoa_parsing();
        c4dtoa_parsing();
        crazy_maya_parsing();
        crazy_sitoa_parsing();
        AiMsgInfo("Cryptomatte unit tests: Name parsing checks complete");
    }
}


void run_all_unit_tests(AtNode *node) {
    if (node 
        && AiNodeLookUpUserParameter(node, CRYPTOMATTE_UNIT_TEST_FLAG) 
        && AiNodeGetBool(node, CRYPTOMATTE_UNIT_TEST_FLAG)) 
    {
        AiMsgWarning("Cryptomatte unit tests: Running");
        NameParsingTests::run();
        AiMsgWarning("Cryptomatte unit tests: Complete");
    }
}


