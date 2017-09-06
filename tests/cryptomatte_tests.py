#
#
#  Copyright (c) 2014, 2015, 2016, 2017 Psyop Media Company, LLC
#  See license.txt
#
#
import tests


def get_all_cryptomatte_tests():
    """ Returns the list of arnold integration tests (Only run in Arnold)"""
    return [CryptomatteTest01]


#############################################
# Cryptomatte test base class
#############################################


class CryptomatteTestBase(tests.KickAndCompareTestCase):
    ass = ""

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

    def assertAllManifestsValidAndMatch(self):
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

    def assertCryptomattePixelsMatch(self,
                                     rms_tolerance=0.01,
                                     very_different_num_tolerance=4,
                                     print_result=False):
        """ 
        Tests pixels match in terms of coverage per ID. Normal image diff doesn't work here with any
        tolerance, because reshuffled IDs (for different sampling) cause giant errors. As a result,
        comparison is more costly, but better geared for Cryptomatte. 
        """
        import math

        def get_id_coverage_dict(pixel_values, ch_pair_idxs):
            return {
                pixel_values[x]: pixel_values[y]
                for x, y, in ch_pair_idxs if (x != 0.0 or y != 0.0)
            }

        big_dif_tolerance = 0.3

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
                            if delta > big_dif_tolerance:
                                very_different_count += 1
                        for id_val, cov in result_id_cov.iteritems():
                            if id_val not in correct_id_cov:
                                total_count += 1
                                delta = cov
                                squared_error += delta * delta
                                if delta > big_dif_tolerance:
                                    very_different_count += 1

            rms = math.sqrt(squared_error / float(total_count))
            if print_result:
                print(self.id(), "Root mean square error: ", rms,
                      "Number of very different pixels: ", very_different_count)

            self.assertTrue(very_different_count < very_different_num_tolerance,
                            "%s matte pixels were very different (max tolerated: %s). " %
                            (very_different_count, very_different_num_tolerance))
            self.assertTrue(rms < 0.01,
                            "Root mean square error was greater than %s. " % rms_tolerance)


#############################################
# Cryptomatte test cases themselves
#############################################


class CryptomatteTest01(CryptomatteTestBase):
    ass = "cryptomatte/01_htoa_instances.ass"

    def test_manifests_valid_and_match(self):
        self.assertAllManifestsValidAndMatch()

    def test_results_all_present(self):
        self.assertAllResultFilesPresent()

    def test_cryptomatte_pixels(self):
        self.assertCryptomattePixelsMatch(print_result=True)
