#ifndef __APS6404L_EXPECTED_HPP
#define __APS6404L_EXPECTED_HPP

#if __cplusplus >= 202302L
    // C++23: Use std::expected directly
    #include <expected>
    namespace psram {
        template<typename T, typename E = std::error_code>
        using expected = std::expected<T, E>;
    }
#else
    // C++17: Use simple expected implementation
    #include <system_error>
    #include <functional>
    #include <utility>
    #include <type_traits>
    #include <stdexcept>

    namespace psram {

    /**
     * @brief Simple expected implementation for C++17 compatibility
     * This is a simplified version of std::expected that will be replaced
     * with std::expected when moving to C++23.
     */

    // Primary template for non-void types
    template<typename T, typename E, typename = void>
    class expected_impl {
    public:
        using value_type = T;
        using error_type = E;

        // Default constructor
        expected_impl() : has_value_(true), value_() {}

        // Constructor from value
        expected_impl(const T& v) : has_value_(true), value_(v) {}
        expected_impl(T&& v) : has_value_(true), value_(std::move(v)) {}

        // Constructor from error
        expected_impl(const E& e) : has_value_(false), error_(e) {}
        expected_impl(E&& e) : has_value_(false), error_(std::move(e)) {}

        // Copy constructor
        expected_impl(const expected_impl& other) : has_value_(other.has_value_) {
            if (has_value_) {
                new (&value_) T(other.value_);
            } else {
                new (&error_) E(other.error_);
            }
        }

        // Move constructor
        expected_impl(expected_impl&& other) : has_value_(other.has_value_) {
            if (has_value_) {
                new (&value_) T(std::move(other.value_));
            } else {
                new (&error_) E(std::move(other.error_));
            }
        }

        // Destructor
        ~expected_impl() {
            if (has_value_) {
                value_.~T();
            } else {
                error_.~E();
            }
        }

        // Copy assignment
        expected_impl& operator=(const expected_impl& other) {
            if (this != &other) {
                if (has_value_) {
                    if (other.has_value_) {
                        value_ = other.value_;
                    } else {
                        value_.~T();
                        new (&error_) E(other.error_);
                        has_value_ = false;
                    }
                } else {
                    if (other.has_value_) {
                        error_.~E();
                        new (&value_) T(other.value_);
                        has_value_ = true;
                    } else {
                        error_ = other.error_;
                    }
                }
            }
            return *this;
        }

        // Move assignment
        expected_impl& operator=(expected_impl&& other) {
            if (this != &other) {
                if (has_value_) {
                    if (other.has_value_) {
                        value_ = std::move(other.value_);
                    } else {
                        value_.~T();
                        new (&error_) E(std::move(other.error_));
                        has_value_ = false;
                    }
                } else {
                    if (other.has_value_) {
                        error_.~E();
                        new (&value_) T(std::move(other.value_));
                        has_value_ = true;
                    } else {
                        error_ = std::move(other.error_);
                    }
                }
            }
            return *this;
        }

        // Check if has value
        explicit operator bool() const noexcept { return has_value_; }
        bool has_value() const noexcept { return has_value_; }

        // Access value
        T& value() & {
            if (!has_value_) throw std::runtime_error("Bad expected access");
            return value_;
        }
        const T& value() const& {
            if (!has_value_) throw std::runtime_error("Bad expected access");
            return value_;
        }
        T&& value() && {
            if (!has_value_) throw std::runtime_error("Bad expected access");
            return std::move(value_);
        }

        // Access error
        E& error() & {
            if (has_value_) throw std::runtime_error("Bad expected access");
            return error_;
        }
        const E& error() const& {
            if (has_value_) throw std::runtime_error("Bad expected access");
            return error_;
        }
        E&& error() && {
            if (has_value_) throw std::runtime_error("Bad expected access");
            return std::move(error_);
        }

        // Value or
        template<typename U>
        T value_or(U&& default_value) const& {
            if (has_value_) return value_;
            return static_cast<T>(std::forward<U>(default_value));
        }

        template<typename U>
        T value_or(U&& default_value) && {
            if (has_value_) return std::move(value_);
            return static_cast<T>(std::forward<U>(default_value));
        }

    private:
        bool has_value_;
        union {
            T value_;
            E error_;
        };
    };

    // Void specialization
    template<typename E>
    class expected_impl<void, E> {
    public:
        using value_type = void;
        using error_type = E;

        // Default constructor (has value by default)
        expected_impl() : has_value_(true) {}

        // Constructor from error
        expected_impl(const E& e) : has_value_(false), error_(e) {}
        expected_impl(E&& e) : has_value_(false), error_(std::move(e)) {}

        // Copy constructor
        expected_impl(const expected_impl& other) : has_value_(other.has_value_) {
            if (!has_value_) {
                new (&error_) E(other.error_);
            }
        }

        // Move constructor
        expected_impl(expected_impl&& other) : has_value_(other.has_value_) {
            if (!has_value_) {
                new (&error_) E(std::move(other.error_));
            }
        }

        // Destructor
        ~expected_impl() {
            if (!has_value_) {
                error_.~E();
            }
        }

        // Copy assignment
        expected_impl& operator=(const expected_impl& other) {
            if (this != &other) {
                if (has_value_) {
                    if (!other.has_value_) {
                        new (&error_) E(other.error_);
                        has_value_ = false;
                    }
                } else {
                    if (other.has_value_) {
                        error_.~E();
                        has_value_ = true;
                    } else {
                        error_ = other.error_;
                    }
                }
            }
            return *this;
        }

        // Move assignment
        expected_impl& operator=(expected_impl&& other) {
            if (this != &other) {
                if (has_value_) {
                    if (!other.has_value_) {
                        new (&error_) E(std::move(other.error_));
                        has_value_ = false;
                    }
                } else {
                    if (other.has_value_) {
                        error_.~E();
                        has_value_ = true;
                    } else {
                        error_ = std::move(other.error_);
                    }
                }
            }
            return *this;
        }

        // Check if has value
        explicit operator bool() const noexcept { return has_value_; }
        bool has_value() const noexcept { return has_value_; }

        // Access value (void - just returns void)
        void value() const {
            if (!has_value_) throw std::runtime_error("Bad expected access");
        }

        // Access error
        E& error() & {
            if (has_value_) throw std::runtime_error("Bad expected access");
            return error_;
        }
        const E& error() const& {
            if (has_value_) throw std::runtime_error("Bad expected access");
            return error_;
        }
        E&& error() && {
            if (has_value_) throw std::runtime_error("Bad expected access");
            return std::move(error_);
        }

        // Value or (for void, returns default constructed T)
        template<typename U>
        void value_or(U&&) const& {}

        template<typename U>
        void value_or(U&&) && {}

    private:
        bool has_value_;
        union {
            E error_;
        };
    };

    /**
     * @brief Simple expected implementation for C++17 compatibility
     * This is a simplified version of std::expected that will be replaced
     * with std::expected when moving to C++23.
     */
    template<typename T, typename E>
    class expected : public expected_impl<T, E> {
    public:
        using base = expected_impl<T, E>;
        using value_type = typename base::value_type;
        using error_type = typename base::error_type;

        // Inherit constructors
        using base::base;
        using base::operator=;

        // Explicit bool conversion is already in base
        // has_value() is already in base
        // value() is already in base
        // error() is already in base
        // value_or() is already in base
    };

    /**
     * @brief Template alias for expected with PSRAM error
     */
    template<typename T>
    using psram_expected = expected<T, std::error_code>;

    } // namespace psram
#endif

#endif // __EXPECTED_HPP
