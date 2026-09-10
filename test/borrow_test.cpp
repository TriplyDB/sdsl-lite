#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <sdsl/construct.hpp>
#include <sdsl/csa_sada.hpp>

#include <gtest/gtest.h>

namespace
{

using csa_type = sdsl::csa_sada<>;

//! Returns the serialized form of x, which is what borrow reads.
template <class T>
std::vector<char> serialized(T const & x)
{
    std::stringstream stream;
    x.serialize(stream);
    std::string const bytes = stream.str();
    // A heap buffer, so the uint64_t members borrow points at are aligned as they are in a file.
    return std::vector<char>(bytes.begin(), bytes.end());
}

//! Builds a CSA and returns its serialized form.
std::vector<char> serialize_csa(csa_type & csa)
{
    std::string text = "abracadabra barbara bark";
    sdsl::construct_im(csa, text, 1);
    return serialized(csa);
}

TEST(borrow, borrows_a_csa_equal_to_the_one_serialized)
{
    csa_type built;
    std::vector<char> const buffer = serialize_csa(built);

    csa_type borrowed;
    std::span<char const> const rest = borrowed.borrow(buffer);

    ASSERT_TRUE(built == borrowed);
    // The structure describes itself completely, so reading it consumes the buffer exactly.
    ASSERT_TRUE(rest.empty());
}

TEST(borrow, throws_on_a_buffer_that_ends_inside_the_structure)
{
    csa_type built;
    std::vector<char> const buffer = serialize_csa(built);

    for (size_t length : {size_t{0}, size_t{8}, buffer.size() / 2, buffer.size() - 1})
    {
        csa_type borrowed;
        ASSERT_THROW((void)borrowed.borrow(std::span(buffer).first(length)), std::out_of_range)
            << "truncated to " << length << " of " << buffer.size() << " bytes";
    }
}

TEST(borrow, throws_on_a_misaligned_buffer)
{
    csa_type built;
    std::vector<char> const serialized = serialize_csa(built);
    std::vector<char> buffer(1);
    buffer.insert(buffer.end(), serialized.begin(), serialized.end());

    csa_type borrowed;
    ASSERT_THROW((void)borrowed.borrow(std::span(buffer).subspan(1)), std::invalid_argument);
}

TEST(borrow, throws_on_a_width_mismatch)
{
    std::vector<char> const buffer = serialized(sdsl::int_vector<>(10, 1, 13));

    sdsl::int_vector<8> borrowed;
    ASSERT_THROW((void)borrowed.borrow(buffer), std::invalid_argument);
}

TEST(borrow, leaves_the_vector_unchanged_when_it_throws)
{
    std::vector<char> const buffer = serialized(sdsl::int_vector<>(100, 7, 32));
    sdsl::int_vector<> owned(50, 3, 32);
    sdsl::int_vector<> const original = owned;

    ASSERT_THROW((void)owned.borrow(std::span(buffer).first(16)), std::out_of_range);
    ASSERT_TRUE(owned == original);
}

TEST(borrow, throws_on_resizing_a_borrowed_vector)
{
    std::vector<char> const buffer = serialized(sdsl::int_vector<>(100, 7, 32));
    sdsl::int_vector<> borrowed;
    (void)borrowed.borrow(buffer);

    ASSERT_THROW(borrowed.push_back(1), std::logic_error);
}

} // namespace
