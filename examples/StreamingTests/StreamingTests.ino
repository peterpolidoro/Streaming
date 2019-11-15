#include <Streaming.h>

// Unit test helper class
class _Assert : public Print
{
public:
    _Assert() : m_idx(0) {}
    size_t      write(uint8_t b) { m_buf[m_idx++] = b; return 1; }
    int8_t      compare(const char * expected) const
    {
        int8_t len_comparison = strlen(expected) - m_idx;
        if ( len_comparison != 0 ) return len_comparison;
        return memcmp(m_buf, expected, m_idx );
    }
    int8_t      compare(const __FlashStringHelper* expected) const
    {
        int8_t len_comparison = strlen_P(reinterpret_cast<const char *>(expected)) - m_idx;
        if ( len_comparison != 0) return len_comparison;
        return memcmp_P(m_buf, expected, m_idx);
    }
    void        reset(void) { m_idx = 0; }
    const char *buffer(void) { m_buf[m_idx] = 0; return m_buf; }

private:
    char        m_buf[40];
    uint8_t     m_idx;
};

_Assert Assert;
uint16_t ran=0;
uint16_t passed=0;

template<typename T>
inline Print& operator==(Print& stm, T expected)
{
    // Hackery since we know stm is of type _Assert
    _Assert* assert = reinterpret_cast<_Assert*>(&stm);
    int8_t cmp = assert->compare(expected);
    // Would love to use the streaming functions here, but as we're testing them we can't
    Serial.print(F("Test "));
    Serial.print(++ran);
    if ( cmp == 0 )
    {
        Serial.println(F(" passed."));
        passed++;
    }
    else
    {
        Serial.print(F(" failed. Expected ["));
        Serial.print(expected);
        Serial.print(F("] but was ["));
        Serial.print(assert->buffer());
        Serial.println(F("]."));
    }
    assert->reset();
    return stm;
}

class PrintableTest : public Printable
{
    size_t printTo(Print& p) const { return p.print("printed"); }
};

void setup()
{
    Serial.begin(115200);
}

void loop()
{
    ran = passed = 0;
    //
    //  Check the assertion comparators
    //
    Assert.print("one");
    if ( Assert.compare("one") != 0 )
    {
        Serial.println(F("const char* comparator failed"));
        while(true);
    }
    if ( Assert.compare(F("one")) != 0)
    {
        Serial.println(F("const __FlashStringHelper* comparator failed"));
        while(true);
    }
    Assert.reset();

    const char abc[] = "abc";
    //
    //  Start by testing that simple types all function correctly
    //  using the generic template
    //
    Assert << 1 == F("1");
    Assert << 10 == F("10");
    Assert << -5 == F("-5");
    Assert << 'a' == F("a");
    Assert << B1000 == F("8");
    Assert << 1.0 == F("1.00");
    Assert << 1.005 == F("1.00");
    Assert << -10.23 == F("-10.23");
    Assert << "abc" == F("abc");
    Assert << abc == F("abc");
    Assert << abc == abc;
    Assert << F("abc") == F("abc");
    Assert << String("string") == F("string");
    Assert << PrintableTest() == F("printed");
    
    //
    //  Check that chaining works
    //
    Assert << 1 << '2' << 0x3 << "4" << F("5") == F("12345");

    //
    //  Now check that the _BASED ones work too
    //
    Assert << _DEC(1) == F("1");
    Assert << _DEC(-1) == F("-1");

    Assert << _DEC((int8_t)1) == F("1");
    Assert << _DEC((int8_t)-1) == F("-1");
    Assert << _DEC((uint8_t)1) == F("1");
    Assert << _DEC((uint8_t)-1) == F("255");
    
    Assert << _DEC((int16_t)1) == F("1");
    Assert << _DEC((int16_t)-1) == F("-1");
    Assert << _DEC((uint16_t)1) == F("1");
    Assert << _DEC((uint16_t)-1) == F("65535");

    Assert << _DEC((int32_t)1) == F("1");
    Assert << _DEC((int32_t)-1) == F("-1");
    Assert << _DEC((uint32_t)1) == F("1");
    Assert << _DEC((uint32_t)-1) == F("4294967295");
    Assert << (uint32_t)4294967295 == F("4294967295");

    Assert << _HEX(15) == F("F");
    Assert << _OCT(15) == F("17");
    Assert << _BIN(15) == F("1111");
    Assert << _BYTE(64) == F("@");

    //
    //  _FLOAT
    //
    double v = 1.23456789012345;
    Assert << _FLOAT(v, 1) == F("1.2");
    Assert << _FLOAT(v, 2) == F("1.23");
    Assert << _FLOAT(v, 3) == F("1.235");
    Assert << _FLOAT(v, 4) == F("1.2346");
    Assert << _FLOAT(v, 5) == F("1.23457");
    Assert << _FLOAT(v, 6) == F("1.234568");

    Assert << _FLOAT(1.7649E22, 5) == F("ovf");    // This really needs work!

    //
    //  endl
    //
    Assert << 1 << endl == F("1\r\n");

    //
    //  Padding helper
    //

    Assert << _PAD(10,' ') == F("          ");
    Assert << _PAD(4, '0') == F("0000");
    Assert << _PAD(20, '=') == F("====================");


    //
    //  Width streaming
    //

    Assert << _WIDTH(1, 5) == F("    1");
    Assert << _WIDTH(-1, 5) == F("   -1");
    Assert << _WIDTHZ(1, 5) == F("00001");
    Assert << _WIDTHZ(128,5) == F("00128");
    Assert << _WIDTHZ(_HEX(128), 5) == F("00080");
    Assert << _WIDTH(abc, 5) == F("  abc");
    Assert << _WIDTH("one", 5) == F("  one");
    Assert << _WIDTH(F("one"),5) == F("  one");

    //
    //  Make sure that all the stream operators return the stream
    //  by chaining them together.
    //

    Assert
        << (int8_t)1            // Generic template
        << _BYTE(50)            // _BYTE_CODE
        << _HEX(3)              // _BASED
        << _FLOAT(4,0)          // _FLOAT
        << _PAD(1,'5')          // Padding

        << endl

        == F("12345\r\n");

    //
    // Final report
    //
    Serial.println();
    Serial.print(F("Tests complete. "));
    Serial.print(ran);
    Serial.print(F(" tests ran. "));
    Serial.print(passed);
    Serial.print(" passed, ");
    Serial.print(ran-passed);
    Serial.println(" failed.");

    delay(1000);
}
