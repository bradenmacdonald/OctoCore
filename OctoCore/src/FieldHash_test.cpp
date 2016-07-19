#include "OctoCore/FieldHash.h"

#include <gtest/gtest.h>
    using ::testing::Test;

namespace testing {

    TEST(FieldHashTest, test_many_strings) {
        EXPECT_EQ(""_octo_field_name_hash, 2166136261);
        EXPECT_EQ("a"_octo_field_name_hash, 3826002220);
        EXPECT_EQ("b"_octo_field_name_hash, 3876335077);
        EXPECT_EQ("c"_octo_field_name_hash, 3859557458);
        EXPECT_EQ("d"_octo_field_name_hash, 3775669363);
        EXPECT_EQ("e"_octo_field_name_hash, 3758891744);
        EXPECT_EQ("f"_octo_field_name_hash, 3809224601);

        EXPECT_EQ("aa"_octo_field_name_hash, 1277494327);
        EXPECT_EQ("it"_octo_field_name_hash, 1194886160);
        EXPECT_EQ("on"_octo_field_name_hash, 1630810064);

        EXPECT_EQ("aaa"_octo_field_name_hash, 876991330);
        EXPECT_EQ("abc"_octo_field_name_hash, 440920331);
        EXPECT_EQ("123"_octo_field_name_hash, 1916298011);

        // Test some long strings:
        EXPECT_EQ("If you truly want to understand something, try to change it."_octo_field_name_hash, 2517180697);
        EXPECT_EQ("An aim in life is the only fortune worth...finding."_octo_field_name_hash, 691148077);

        // Test UTF-8
        EXPECT_EQ("theta is Θ or Ө or θ"_octo_field_name_hash, 20395768);

        // Common field names
        EXPECT_EQ("id"_octo_field_name_hash, 926444256);
        EXPECT_EQ("name"_octo_field_name_hash, 2369371622);
        EXPECT_EQ("value"_octo_field_name_hash, 1113510858);
        EXPECT_EQ("type"_octo_field_name_hash, 1361572173);
        EXPECT_EQ("email"_octo_field_name_hash, 2324124615);
        EXPECT_EQ("age"_octo_field_name_hash, 742476188);
        EXPECT_EQ("enabled"_octo_field_name_hash, 49525662);
        EXPECT_EQ("disabled"_octo_field_name_hash, 871591685);
        EXPECT_EQ("active"_octo_field_name_hash, 3648362799);

        // And double-check that this is all working at compile-time:
        static_assert("enabled"_octo_field_name_hash == 49525662, "Field hashing must work at compile-time.");
    }

} // namespace testing
