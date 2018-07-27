#
#
#  Copyright (c) 2014, 2015, 2016, 2017 Psyop Media Company, LLC
#  See license.txt
#
#

import os
import unittest
import subprocess
try:
    import OpenImageIO as oiio
    from OpenImageIO import ImageBuf, ImageSpec, ImageBufAlgo
except:
    oiio = None



def get_all_arnold_tests():
    """ Returns the list of arnold integration tests (Only run in Arnold)"""
    return [] + get_all_cryptomatte_tests()


def get_all_cryptomatte_tests():
    """ Returns the list of arnold integration tests (Only run in Arnold)"""
    import cryptomatte_tests
    return cryptomatte_tests.get_all_cryptomatte_tests()


#############################################
# KickAndCompare base class
#############################################

class KickAndCompareTestCase(unittest.TestCase):
    ass = ""
    arnold_v = 1
    arnold_t = 4

    @classmethod
    def setUpClass(self):
        assert self.ass, "No test name specified on test."

        file_dir = os.path.abspath(os.path.dirname(__file__))

        build_dir = os.path.normpath(os.path.join(file_dir, "..", "build")).replace("\\", "/")
        print build_dir
        if not os.path.exists(build_dir):
            raise RuntimeError("could not find %s ", build_dir)
        self.ass_file = os.path.join(file_dir, self.ass)
        ass_file_name = os.path.basename(self.ass_file)
        test_dir = os.path.abspath(os.path.dirname(self.ass_file))

        self.result_dir = os.path.join(test_dir, "%s_result" % ass_file_name[:3]).replace("\\", "/")
        self.correct_result_dir = os.path.join(test_dir, "%s_correct" % ass_file_name[:3]).replace("\\", "/")
        self.result_log = os.path.join(self.result_dir, "log.txt").replace("\\", "/")
        self.correct_file_names = [
            x for x in os.listdir(self.correct_result_dir)
            if os.path.isfile(os.path.join(self.correct_result_dir, x))
        ]

        assert os.path.isfile(self.ass_file), "No test ass file found. %s" % (self.ass_file)
        assert os.path.isdir(test_dir), "No test dir found. %s" % (test_dir)
        assert os.path.isdir(self.correct_result_dir), "No correct result dir found. %s" % (
            self.correct_result_dir)

        # only remove previous results after it's confirmed everything else exists, to
        # mitigate odds we're looking at the wrong dir or something.
        if os.path.exists(self.result_dir):
            assert os.path.isdir(self.result_dir), "result directory is not a directory"
        else:
            os.mkdir(self.result_dir)

        for file_name, result_file in ((x, os.path.join(self.result_dir, x))
                                       for x in os.listdir(self.result_dir)):
            if os.path.isfile(result_file):
                os.remove(os.path.join(self.result_dir, file_name))

        remaining_files = [
            file_name for file_name in os.listdir(self.result_dir)
            if os.path.isfile(os.path.join(self.result_dir, file_name))
        ]
        assert not remaining_files, "Files were not cleaned up: %s " % remaining_files

        cmd = 'kick -v {v} -t {t} -dp -dw -sl -nostdin -logfile {log} -i {ass}'.format(
            v=self.arnold_v, t=self.arnold_t, log=self.result_log, ass=ass_file_name)
        cwd = test_dir.replace("\\", "/")
        print cmd, cwd
        env = os.environ.copy()
        env["ARNOLD_PLUGIN_PATH"] = "%s;%s" % (build_dir, env.get("ARNOLD_PLUGIN_PATH", ""))
        proc = subprocess.Popen(cmd, cwd=cwd, env=env, shell=True, stderr=subprocess.PIPE)
        rc = proc.wait()
        assert rc == 0, "Render return code indicates a failure: %s " % rc

    #
    # Helpers
    #
            
    def fail_test_if_no_oiio(self):
        if oiio is None:
            self.fail("OIIO not loaded.")

    def load_images(self, file_name):
        self.fail_test_if_no_oiio()
        allowed_exts = {".exr", ".tif", ".png", ".jpg"}
        result_file = os.path.join(self.result_dir, file_name)
        correct_result_file = os.path.join(self.correct_result_dir, file_name)
        if os.path.splitext(result_file)[1] not in allowed_exts:
            return None, None
        result_image = ImageBuf(result_file)
        correct_result_image = ImageBuf(correct_result_file)
        return result_image, correct_result_image

    def assertSameChannels(self, result_image, correct_result_image):
        r_channels = set(result_image.spec().channelnames)
        c_channels = set(correct_result_image.spec().channelnames)
        self.assertEqual(r_channels, c_channels,
                         "Channels mismatch between result and correct. %s vs %s" % (r_channels,
                                                                                     c_channels))

    def compare_image_pixels(self, result_image, correct_result_image, threshold):
        self.fail_test_if_no_oiio()
        """ 
        Given a file to find it results, compare it against the correct result. 
        Returns None if the input is not an image. 
        Returns oiio.CompareResults if results can be compared. 
        """
        compresults = oiio.CompareResults()
        ImageBufAlgo.compare(result_image, correct_result_image, threshold, threshold, compresults)
        return compresults

    #
    # Assertions, for use in other test cases
    #

    def assertAllResultFilesPresent(self):
        """ Checks there are no files in result not in correct, and vice versa """
        for file_name in os.listdir(self.correct_result_dir):
            result_file = os.path.join(self.result_dir, file_name)
            self.assertTrue(
                os.path.isfile(result_file),
                'Result file "%s" not found for correct answer file. %s' % (file_name,
                                                                            self.result_dir))
        for file_name in os.listdir(self.result_dir):
            correct_result_file = os.path.join(self.correct_result_dir, file_name)
            self.assertTrue(
                os.path.isfile(correct_result_file),
                'Correct answer "%s" not found for result file. %s' % (file_name,
                                                                       self.correct_result_dir))

    def assertResultImageEqual(self, file_name, threshold, msg, print_results=True):
        """ 
        Given a file name to find it results, compare it against the correct result. 
        """
        result, correct_result = self.load_images(file_name)
        if not result or not correct_result:
            return
        self.assertSameChannels(result, correct_result)
        compresults = self.compare_image_pixels(result, correct_result, threshold)
        if compresults is None:
            return
        if compresults.maxerror == 0.0:
            result_msg = file_name + " - Perfect match."
        else:
            result_msg = file_name + " - meanerror: %s rms_error: %s PSNR: %s maxerror: %s " % (
                compresults.meanerror, compresults.rms_error, compresults.PSNR,
                compresults.maxerror)

        self.assertEqual(compresults.nfail, 0,
                         "%s. Did not match within threshold %s. %s" % (msg, file_name, result_msg))
        # if print_results:
        #     print("Passed: (within tolerance) - ", result_msg)

    def assertAllResultImagesEqual(self, tolerance):
        """ Checks all correct images match results """
        for file_name in self.correct_file_names:
            self.assertResultImageEqual(file_name, tolerance, "")


#############################################
# Test cases themselves
#############################################


def run_arnold_tests(test_filter=""):
    """ Utility function for manually running tests inside Nuke
    Returns unittest results if there are failures, otherwise None """
    return run_tests(get_all_arnold_tests(), test_filter)


def run_tests(test_cases, test_filter=""):
    """ Utility function for manually running tests. 
    Returns results if there are failures, otherwise None 

    test_filter will be matched fnmatch style (* wildcards) to either the name of the TestCase 
    class or test method. 

    """
    import fnmatch

    def find_test_method(traceback):
        """ Finds first "test*" function in traceback called. """
        import re
        match = re.search('", line \d+, in (test[a-z_0-9]+)', traceback)
        return match.group(1) if match else ""

    result = unittest.TestResult()
    suite = unittest.TestSuite()
    for case in test_cases:
        suite.addTests(unittest.TestLoader().loadTestsFromTestCase(case))

    if test_filter:
        filtered_suite = unittest.TestSuite()
        for test in suite:
            if fnmatch.fnmatch(test.id(), test_filter):
                filtered_suite.addTest(test)
            elif any(fnmatch.fnmatch(x, test_filter) for x in test.id().split(".")):
                filtered_suite.addTest(test)
        if not any(True for _ in filtered_suite):
            raise RuntimeError("Filter %s selected no tests. " % test_filter)
        suite = filtered_suite

    suite.run(result)
    print "---------"
    for test_instance, traceback in result.failures:
        print "Failed: %s.%s" % (type(test_instance).__name__, find_test_method(traceback))
        print
        print traceback
        print "---------"
    for test_instance, traceback in result.errors:
        print "Error: %s.%s" % (type(test_instance).__name__, find_test_method(traceback))
        print
        print traceback
        print "---------"

    if result.failures or result.errors:
        print "TESTING FAILED: %s failed, %s errors. (%s test cases.)" % (len(result.failures),
                                                                          len(result.errors),
                                                                          suite.countTestCases())
        return result
    else:
        print "Testing passed: %s failed, %s errors. (%s test cases.)" % (len(result.failures),
                                                                          len(result.errors),
                                                                          suite.countTestCases())
        return None
