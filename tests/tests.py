#
#
#  Copyright (c) 2014, 2015, 2016, 2017 Psyop Media Company, LLC
#  See license.txt
#
#

import unittest
import subprocess
import os
import OpenImageIO as oiio
from OpenImageIO import ImageBuf, ImageSpec, ImageBufAlgo


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

    @classmethod
    def setUpClass(self):
        assert self.ass, "No test name specified on test."

        file_dir = os.path.abspath(os.path.dirname(__file__))
        self.ass_file = os.path.join(file_dir, self.ass)
        ass_file_name = os.path.basename(self.ass_file)
        test_dir = os.path.abspath(os.path.dirname(self.ass_file))

        self.result_dir = os.path.join(test_dir, "%s_result" % ass_file_name[:3])
        self.correct_result_dir = os.path.join(test_dir, "%s_correct" % ass_file_name[:3])
        self.result_log = os.path.join(self.result_dir, "log.txt")
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

        cmd = 'kick -v 2 -t 2 -dp -dw -sl -logfile %s %s' % (self.result_log, ass_file_name)
        cwd = test_dir.replace("\\", "/")
        print cmd, cwd
        proc = subprocess.Popen(cmd, cwd=cwd, shell=True, stderr=subprocess.PIPE)
        rc = proc.wait()
        assert rc == 0, "Render return code indicates failure: %s " % rc

    #
    # Helpers
    #

    def load_images(self, file_name):
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
        if print_results:
            print("Passed: (within tolerance) - ", result_msg)

    def assertAllResultImagesEqual(self, tolerance):
        """ Checks all correct images match results """
        for file_name in self.correct_file_names:
            self.assertResultImageEqual(file_name, tolerance, "")


#############################################
# Test cases themselves
#############################################


class CryptomatteTest01(KickAndCompareTestCase):
    ass = "01_htoa_instances.ass"

    def setUp(self):
        self.result_images = []
        for file_name in self.correct_file_names:
            img, correct_img = self.load_images(file_name)
            if img and correct_img:
                self.result_images.append((img, correct_img))

    def cryptomatte_metadata(self, ibuf):
        """Returns dictionary of key, value of cryptomatte metadata"""
        return {
            a.name: a.value
            for a in ibuf.spec().extra_attribs if a.name.startswith("cryptomatte")
        }

    def sorted_cryptomatte_metadata(self, img):
        """
        Gets a dictionary of the cryptomatte metadata, interleved by cryptomatte stream. 

        for example:
            {"crypto_object": {"name": crypto_object", ... }}

        Also includes ID coverage pairs in subkeys, "ch_pair_idxs" and "ch_pair_names". 
        """
        img_md = self.cryptomatte_metadata(img)
        cryptomatte_streams = {}
        for key, value in img_md.iteritems():
            prefix, cryp_key, cryp_md_key = key.split("/")
            name = img_md["/".join((prefix, cryp_key, "name"))]
            cryptomatte_streams[name] = cryptomatte_streams.get(name, {})
            cryptomatte_streams[name][cryp_md_key] = value

        for cryp_key in cryptomatte_streams:
            name = cryptomatte_streams[cryp_key]["name"]
            ch_id_coverages = []
            ch_id_coverage_names = []
            channels_dict = {ch: i for i, ch in enumerate(img.spec().channelnames)}
            for i, ch in enumerate(img.spec().channelnames):
                if not ch.startswith(name):
                    continue
                if ch.startswith("%s." % name):
                    continue
                if ch.endswith(".R"):
                    red_name = ch
                    green_name = "%s.G" % ch[:-2]
                    blue_name = "%s.B" % ch[:-2]
                    alpha_name = "%s.A" % ch[:-2]

                    red_idx = i
                    green_idx = channels_dict[green_name]
                    blue_idx = channels_dict[blue_name]
                    alpha_idx = channels_dict[alpha_name]

                    ch_id_coverages.append((red_idx, green_idx))
                    ch_id_coverages.append((blue_idx, alpha_idx))
                    ch_id_coverage_names.append((red_name, green_name))
                    ch_id_coverage_names.append((blue_name, alpha_name))
            cryptomatte_streams[cryp_key]["ch_pair_idxs"] = ch_id_coverages
            cryptomatte_streams[cryp_key]["ch_pair_names"] = ch_id_coverage_names
        return cryptomatte_streams

    def assertManifestsAreValidAndMatch(self, result_md, correct_md, key):
        """ Does a comparison between two manifests. Order is not important, but contents are. 
        Checks both are parsable with json
        Checks that there are no extra names in either manifest
        """
        import json
        try:
            correct_manifest = json.loads(correct_md[key])
        except Exception, e:
            raise RuntimeError("Correct manifest could not be loaded. %s" % e)
        try:
            result_manifest = json.loads(result_md[key])
        except Exception, e:
            self.fail("Result manifest could not be loaded. %s" % e)

        # test manifest hashes?
        correct_names = set(correct_manifest.keys())
        result_names = set(result_manifest.keys())

        extra_in_correct = correct_names - result_names
        extra_in_result = result_names - correct_names
        if extra_in_correct:
            self.fail("Missing in result manifest: %s" % extra_in_correct)
        if extra_in_result:
            self.fail("Extra in result manifest: %s" % extra_in_result)

    def test_result_files_all_present(self):
        self.assertAllResultFilesPresent()

    def test_cryptomatte_manifests(self):
        """ 
        Tests manifests match and are valid, and tests that all other cryptomatte 
        metadata is equal. 
        """
        for result_img, correct_img in self.result_images:
            self.assertSameChannels(result_img, correct_img)
            result_md = self.cryptomatte_metadata(result_img)
            correct_md = self.cryptomatte_metadata(correct_img)

            for key in correct_md:
                self.assertIn(key, result_md, "Result missing metadata key: %s" % key)
            for key in result_md:
                self.assertIn(key, correct_md, "Result has extra metadata key: %s" % key)

            for key in correct_md:
                if key.endswith("/manifest"):
                    self.assertManifestsAreValidAndMatch(result_md, correct_md, key)
                else:
                    self.assertEqual(correct_md[key], result_md[key],
                                     "Metadata doesn't match: %s vs %s " % (result_md[key],
                                                                            correct_md[key]))

    def test_cryptomatte_pixels(self):
        import math

        def get_id_coverage_dict(pixel_values, ch_pair_idxs):
            return {
                pixel_values[x]: pixel_values[y]
                for x, y, in ch_pair_idxs if (x != 0.0 or y != 0.0)
            }

        rms_threshold = 0.01
        very_different_threshold = 0.3
        max_very_different_pixels = 4

        for result_img, correct_img in self.result_images:
            result_nested_md = self.sorted_cryptomatte_metadata(result_img)
            correct_nested_md = self.sorted_cryptomatte_metadata(correct_img)

            total_count = 0
            very_different_count = 0
            squared_error = 0.0
            for row in range(0, 128):
                for column in range(0, 128):
                    result_pixel = result_img.getpixel(row, column)
                    correct_pixel = correct_img.getpixel(row, column)

                    for cryp_key in result_nested_md:
                        result_id_cov = get_id_coverage_dict(
                            result_pixel, result_nested_md[cryp_key]["ch_pair_idxs"])
                        correct_id_cov = get_id_coverage_dict(
                            correct_pixel, correct_nested_md[cryp_key]["ch_pair_idxs"])
                        for id_val, cov in correct_id_cov.iteritems():
                            total_count += 1
                            delta = abs(cov - (result_id_cov.get(id_val, 0.0)))
                            squared_error += delta * delta
                            if delta > very_different_threshold:
                                very_different_count += 1
                        for id_val, cov in result_id_cov.iteritems():
                            if id_val not in correct_id_cov:
                                total_count += 1
                                delta = cov
                                squared_error += delta * delta
                                if delta > very_different_threshold:
                                    very_different_count += 1

            rms = math.sqrt(squared_error / float(total_count))
            print("Root mean square error: ", rms, "Number of very different pixels: ",
                  very_different_count)
            self.assertTrue(rms < 0.01,
                            "Root mean square error was greater than %s. " % rms_threshold)

            self.assertTrue(very_different_count < max_very_different_pixels,
                            "%s matte pixels were very different (max tolerated: %s). " %
                            (very_different_count, max_very_different_pixels))


def run_arnold_tests(id_filter=""):
    """ Utility function for manually running tests inside Nuke
    Returns unittest results if there are failures, otherwise None """
    return run_tests(get_all_arnold_tests(), id_filter)


def run_tests(test_cases, id_filter=""):
    """ Utility function for manually running tests inside Nuke. 
    Returns results if there are failures, otherwise None """
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

    if id_filter:
        filtered_suite = unittest.TestSuite()
        for test in suite:
            if any(fnmatch.fnmatch(x, id_filter) for x in test.id().split(".")):
                filtered_suite.addTest(test)
        suite = filtered_suite

    suite.run(result)
    print "---------"
    for test_instance, traceback in result.failures:
        print "%s Failed: " % type(test_instance).__name__, find_test_method(traceback)
        print
        print traceback
        print "---------"
    for test_instance, traceback in result.errors:
        print "%s Error: " % type(test_instance).__name__, find_test_method(traceback)
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
