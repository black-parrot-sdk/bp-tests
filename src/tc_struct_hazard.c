#include <stdio.h>
#include <stdint.h>
#include "bp_utils.h"
#include "bp_asm.h"

#define CEIL_DIV(A,D) (((A)+(D-1))/(D))


//#define USE_FOR_LOOP
#define USE_TC

//#define TEST_CASE_1
//#define TEST_CASE_2
//#define TEST_CASE_3
#define TEST_CASE_4

#if defined(TEST_CASE_1)
    #define I 4
    #define J 4
    #define K 4

    #define I_PRIME 1
    #define J_PRIME 1
    #define K_PRIME 1

    #define BUF_SIZE (128*128)

    volatile uint32_t A        [BUF_SIZE] __attribute__ ((aligned (64))) __attribute__ ((section (".data"))) = {1,1,1,1,2,2,2,2,4,4,4,4,8,8,8,8};
    volatile uint32_t W        [BUF_SIZE] __attribute__ ((aligned (64))) __attribute__ ((section (".data"))) = {16,32,64,128,16,32,64,128,16,32,64,128,16,32,64,128};
    volatile uint32_t result   [BUF_SIZE] __attribute__ ((aligned (64))) __attribute__ ((section (".data"))) = {0};
    volatile uint32_t expected [BUF_SIZE] __attribute__ ((aligned (64))) __attribute__ ((section (".data"))) = {255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255};

#elif defined(TEST_CASE_2)
    #define I 8
    #define J 4
    #define K 8

    #define I_PRIME 2
    #define J_PRIME 1
    #define K_PRIME 2

    volatile uint32_t A[1024]        __attribute__ ((aligned (64))) __attribute__ ((section (".data"))) = {1654615998,1806341205,173879092,1112038970,2087043557,1739178872,1302718217,2046968324,1537810351,938204377,598176026,1210484339,600203567,407295012,1075916535,631194409,1332073689,424185324,316721330,1418186270,2027837527,432508404,1519522183,1864753826,1358054485,878225224,2048741382,1901353491,1118805955,267488770,60308648,400599612};
    volatile uint32_t W[1024]        __attribute__ ((aligned (64))) __attribute__ ((section (".data"))) = {1712934065,4908357,2119900799,1430804514,952210990,1024847607,612008755,1924014660,468399889,1294704107,1250224899,536057929,393556845,1653137829,1361705852,1039842312,1047589226,1396742090,270515404,820626892,391769539,345512145,1374624034,2101470722,1429152570,872745308,1235478542,1911213317,1246955724,789710164,813429542,801997237};
    volatile uint32_t result[1024]   __attribute__ ((aligned (64))) __attribute__ ((section (".data"))) = {0};
    volatile uint32_t expected[1024] __attribute__ ((aligned (64))) __attribute__ ((section (".data"))) = {325473286,1406991641,11274198,1154471106,2116909427,1050136684,1843866275,696580535,1434839804,356467683,1179629868,36465208,209192502,1287827241,531781094,1540458226,2004204986,502204918,2030580771,1387259291,440307919,1889805443,339941718,1071958254,831914816,1527559948,1072401113,340548449,1751904138,49372102,1712743955,1303492523,1108115834,45492325,1371984554,363135422,115931237,1179799930,354593717,1363245217,1034237931,2099224308,779052091,1779218223,190863798,1272639657,413796966,1556732274,639592646,1283841162,677568863,63254759,1660621273,140617109,1820760128,1193363960,1505156695,858548763,1472944078,2087487094,1869852682,99157062,1628322195,1252649003};

#elif defined(TEST_CASE_3)
    #define I 8
    #define J 8
    #define K 8

    #define I_PRIME 2
    #define J_PRIME 2
    #define K_PRIME 2

    volatile uint32_t A[64] __attribute__ ((aligned (64))) __attribute__ ((section (".data"))) = {1654615998,1806341205,173879092,1112038970,1537810351,938204377,598176026,1210484339,1332073689,424185324,316721330,1418186270,1358054485,878225224,2048741382,1901353491,2087043557,1739178872,1302718217,2046968324,600203567,407295012,1075916535,631194409,2027837527,432508404,1519522183,1864753826,1118805955,267488770,60308648,400599612,1712934065,4908357,2119900799,1430804514,952210990,1024847607,612008755,1924014660,468399889,1294704107,1250224899,536057929,393556845,1653137829,1361705852,1039842312,1047589226,1396742090,270515404,820626892,391769539,345512145,1374624034,2101470722,1429152570,872745308,1235478542,1911213317,1246955724,789710164,813429542,801997237};
    volatile uint32_t W[64] __attribute__ ((aligned (64))) __attribute__ ((section (".data"))) = {141615452,1116932915,2046685052,296718909,344663896,1680603714,1183839549,1011472239,2115938757,1534881946,353789296,1392964597,1043830061,69617246,1164099199,503087954,385792672,559309739,642313784,165964262,924282500,1801409089,1182021441,1935153793,495423391,2089261940,1439975734,817670926,946870804,1597986360,732213632,1428231901,1830219288,267114565,432105997,628604922,534481714,809689893,514088150,1680231637,92928119,835761764,794448426,532125690,1828018965,435940125,1116347426,300666185,939625287,194278833,317814271,114661864,393126321,1589738942,498414388,156302611,2058292873,904419226,262357606,97850419,948454521,309116047,1293049471,1504501144};
    volatile uint32_t expected[64] __attribute__ ((aligned (64))) __attribute__ ((section (".data"))) = {937058776,1005175177,1189011267,1667632652,1167007389,1237106380,881565702,288584009,1327489230,1126973599,1041981013,463703834,1117208861,1319428428,865337222,371928777,1092863894,1589265546,1637809474,2140605120,863418579,753875919,332104199,230847877,971392640,645907868,425426004,122850262,879683411,737633359,349429127,182069765,1450695144,1514437049,661874547,46904892,887016570,955083819,1171944161,1617077166,1121110309,1323395444,869693374,376350449,1632024899,1836390674,272762840,905631383,546209702,1058536634,3360114,507163376,1109879348,1572792616,1654280416,2090051426,875261803,733146231,345396671,178233917,398401293,136748049,922887641,688640603};
    volatile uint32_t result[64] __attribute__ ((aligned (64))) __attribute__ ((section (".data")));

#elif defined(TEST_CASE_4)
    #define I 16
    #define J 16
    #define K 16

    #define I_PRIME 4
    #define J_PRIME 4
    #define K_PRIME 4

    volatile uint32_t A[256] __attribute__ ((aligned (64))) __attribute__ ((section (".data"))) = {1654615998,1806341205,173879092,1112038970,1332073689,424185324,316721330,1418186270,1712934065,4908357,2119900799,1430804514,468399889,1294704107,1250224899,536057929,2087043557,1739178872,1302718217,2046968324,2027837527,432508404,1519522183,1864753826,1047589226,1396742090,270515404,820626892,1429152570,872745308,1235478542,1911213317,1537810351,938204377,598176026,1210484339,1358054485,878225224,2048741382,1901353491,952210990,1024847607,612008755,1924014660,393556845,1653137829,1361705852,1039842312,600203567,407295012,1075916535,631194409,1118805955,267488770,60308648,400599612,391769539,345512145,1374624034,2101470722,1246955724,789710164,813429542,801997237,141615452,1116932915,2046685052,296718909,2115938757,1534881946,353789296,1392964597,1830219288,267114565,432105997,628604922,92928119,835761764,794448426,532125690,385792672,559309739,642313784,165964262,495423391,2089261940,1439975734,817670926,939625287,194278833,317814271,114661864,2058292873,904419226,262357606,97850419,344663896,1680603714,1183839549,1011472239,1043830061,69617246,1164099199,503087954,534481714,809689893,514088150,1680231637,1828018965,435940125,1116347426,300666185,924282500,1801409089,1182021441,1935153793,946870804,1597986360,732213632,1428231901,393126321,1589738942,498414388,156302611,948454521,309116047,1293049471,1504501144,1872900585,774459493,262175295,2006313350,1470221169,1076693935,503425332,1899724293,1442750606,196264944,1206384019,579123108,1436244341,687537412,1028283729,958197830,169130250,433453424,1680518472,856226606,750843993,56709708,2025615733,1760530271,1031352225,2069431579,1512797275,1236426221,1924726585,1626281073,1779861933,135486808,1117263813,1539983294,2019563010,727673138,1337650368,1533954791,1669205724,1077747587,1542757380,568414356,1332717957,1666521335,1727723913,1796269370,200917231,711568583,873659410,249326891,679500893,695587449,658902818,53413578,1966898329,339619332,1779911708,346703132,6546479,825963191,1912773091,274389611,1113371136,677258676,1917121846,2092789797,297036,167117967,1357970475,14572021,917081460,61380729,93825718,1179886944,1945526488,497268706,1575796842,180408077,2123799795,1969948437,2124216399,1399918423,1340279962,2005286885,10125659,419980565,818019178,510688338,1101610896,573036909,1490581366,494390848,1870203225,1599657397,765742497,892683641,214134156,1783001626,807459420,358488241,852621339,1298867023,1202516726,782633881,663315337,1195761054,79776130,181666837,1613099182,1249987923,38212703,594668076,560586626,63261809,1725790100,1793084754,430272795,2042709959,1703797118,349375932,174647438,883628285,1115271501,1351683036,648639589,1165540019,1431978117,1449598999};
    volatile uint32_t W[256] __attribute__ ((aligned (64))) __attribute__ ((section (".data"))) = {1577118317,402470710,1452798332,153182031,207259503,1322333523,771319623,304353766,2057660952,498250834,2137753491,1831955274,1652754537,33719836,421235423,1691392275,176965319,1158018294,703775687,641736990,1299960583,1731598297,1410985724,1285163745,162454480,1296664028,1439165853,668572195,1244770883,1926483037,2098601769,933096865,1243525559,1550082102,1695618646,556803596,1781215744,466654280,426921187,2066619266,715353335,1613066649,373326185,282757781,1816934709,359330904,1581971554,945484899,1260173501,493476348,2053272110,1029529788,2035834473,1447749897,1475972382,533970967,363653025,850576530,949439989,262663685,1120627997,715460430,1852080195,824397566,1539833323,493974501,274267788,118622481,1968847553,1623634925,1553240961,649976180,1461235106,1366913872,1382227385,152335021,174513230,1290415600,65649154,1926675488,1939657864,866243010,510715801,2134974445,449881842,2095520584,637381822,1743356771,637230909,1103299417,669608418,1627814540,1419891973,690158552,639174841,1978842144,1709197715,1101571809,890054541,180824244,1817797142,2127752239,1384956481,2140787315,1264362550,2020487407,285010301,363539489,1594815264,1641676345,144388256,389325420,927259236,628587760,449381102,850274848,2141670794,867546062,939623321,41681215,169289719,284991214,966757007,560468097,327955552,1831230936,885213514,1244098010,1794611922,2071091467,1668890677,1003180211,1647890592,267399352,703495997,546838737,1765833703,151096046,2002652867,1660790868,1646750908,2105906981,476728158,258388781,87968578,1033789,781550570,1298978690,1026289730,1233035034,1434624957,238486822,1970884634,202004839,435855470,2022453750,2006558946,1451120738,533966599,1272969853,1092894811,1428874698,281839377,2119609405,154334646,2067298395,1794720992,605181853,650119556,86878931,139438105,569950550,545929534,1664925561,1261876549,522063593,1125313762,1300537029,1752660907,1649953459,2112397876,350553676,650145383,1515143493,1391188677,452229247,1488927646,837405312,812296305,164000791,1683740534,1909454868,1595931004,818094883,1956229772,1531386947,625735598,1829528259,1812718940,360914926,1393713149,1084848319,337320475,1513988967,893258886,13881775,879569790,529088950,323581050,191731217,171715695,2088063218,449960618,1785165347,270251514,426107957,302062762,521314537,1542981238,126175447,30857475,1259554806,1585401916,105870989,1096875922,114486652,927624393,986361730,1783104834,670689064,131859121,1918641781,1483675425,1494123482,764107585,42796075,1000139413,609401913,803123253,1950121629,401477296,1804363506,1311303284,487242132,1851501600,1791085173,129425550,2132987154,989962187,1571109688,302870822,615430777,482715808,2047384744,1479399794,1109041502};
    volatile uint32_t expected[256] __attribute__ ((aligned (64))) __attribute__ ((section (".data"))) = {1111917948,1133195301,1725611616,1431189929,1163844348,1150513061,1640161760,1381393961,857167771,853205698,395013255,606071630,1720656546,1732417531,1108548030,1904713335,1148302252,1798692856,1100879626,988041800,1131015212,1814921336,1183205514,1039928776,891314507,442764575,818922989,1270186159,1622676594,1341945894,1700245716,506328470,384163761,38399330,1828543961,1612262292,301826097,89240290,1810170457,1728148500,1739853142,1930968965,497848126,289995123,841775215,645941948,1211431431,1154518090,5197102,2027869122,1568700027,1520613236,123146926,2143718466,1519921659,1572530420,1897832393,160366885,751936668,737371539,612821744,1545040924,2034880933,2120945834,1964788908,1959918069,1367373744,1645539449,95598296,75432833,556699076,314182157,1093816193,1090517720,1705931933,1446703956,572663480,602903521,113122724,891984493,1932229244,1550784040,1992613594,230595480,59080712,750906460,107619502,2098225644,1191541073,1749095685,1122688503,965777589,605360232,189889596,570078414,1518399884,566171233,890447026,1537709065,1464066628,1360129045,1170071238,722038397,669849648,361829708,20785055,1871288100,1668145513,1988197493,1647136422,211418653,8226896,924157182,1334057490,1792720811,1845001892,1203469962,1059774566,443827679,492209360,54233043,2074745151,1593097350,1507229065,1613491946,414871558,1038407103,986059952,658719412,646903789,64977320,810286689,1494419164,1490989957,2106539456,1310439945,1402831409,1380988776,1996601645,1150631652,337807765,368000204,817803913,53097792,561280100,238032944,614191298,1608619392,1596207116,1885363288,1523291306,565301736,1437250785,2062473397,1346703431,725570821,303415109,1028772625,402267107,1820352161,1944066169,1733044906,167287313,85781596,229661713,421293762,2007555705,2068621364,121559292,328395311,2099665556,1908550873,1082250072,1412293003,983126320,914171773,1699384038,500896778,948285875,1067937980,454741646,1670074466,1188427227,1106719956,294931043,1761989775,1281061174,1266266169,1445675463,783998763,199522962,214350749,854193308,858014149,376819584,635521097,222463927,210711278,702463147,441072482,810751819,832479762,349121623,660319134,2090136557,2103002804,1476970737,1805406008,887013964,463121944,825321194,1246438312,192262503,607247667,244979137,1977634947,912531867,421547471,864781629,1223803007,2057553213,1441245545,2135712155,70657241,1716170321,1927715970,475147321,280386164,1507974522,1296953257,603545362,790440287,1693470086,1885110101,518574062,302823843,674736416,1016830963,1378822984,1590385925,1893743822,141664802,758056859,705545876,1330335717,937087241,310670512,363247039,1918557977,181092853,798352460,683091267,1050491839,1175233875,1666318570,1685111269};
    volatile uint32_t result[256] __attribute__ ((aligned (64))) __attribute__ ((section (".data")));
#endif



#define BP_PRINTF_MAXLEN 1024

char bp_printf_buf[BP_PRINTF_MAXLEN];

#define bp_printf(fmt, ...) \
    snprintf(bp_printf_buf, BP_PRINTF_MAXLEN, fmt, __VA_ARGS__); \
    bp_print_string(bp_printf_buf);

#define nop() ({ asm volatile ("nop"); })

#define read_csr(reg) ({ \
    unsigned long __tmp; \
    asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
    __tmp; \
})

#define matmul1_b0(wt,addr) \
     __asm__ __volatile__ ( \
         ".insn i 0x0b, 0b000, %0, %0, 64\n\t" \
         ".insn i 0x0b, 0b010, %1, %1, 64\n\t" \
         "nop\n\t" \
         "nop\n\t" \
         "nop\n\t" \
        : "+r" (wt), "+r" (addr) : \
     );

#define matmul1_b1(wt,addr) \
    __asm__ __volatile__ ( \
        ".insn i 0x0b, 0b001, %0, %0, 64\n\t" \
        ".insn i 0x0b, 0b011, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        : "+r" (wt), "+r" (addr) : \
    );

#define matmul2_b0(wt,addr) ({ \
    __asm__ __volatile__ ( \
        ".insn i 0x0b, 0b000, %0, %0, 64\n\t" \
        ".insn i 0x0b, 0b010, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b010, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        : "r+" (wt), "r+" (addr) : \
    ); \
})

#define matmul2_b1(wt,addr) ({ \
    __asm__ __volatile__ ( \
        ".insn i 0x0b, 0b001, %0, %0, 64\n\t" \
        ".insn i 0x0b, 0b011, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b011, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        : "r+" (wt), "r+" (addr) : \
    ); \
})

#define matmul4_b0(wt,addr) ({ \
    __asm__ __volatile__ ( \
        ".insn i 0x0b, 0b000, %0, %0, 64\n\t" \
        ".insn i 0x0b, 0b010, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b010, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b010, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b010, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        : "r+" (wt), "r+" (addr) + \
    ); \
})

#define matmul4_b1(wt,addr) ({ \
    __asm__ __volatile__ ( \
        ".insn i 0x0b, 0b001, %0, %0, 64\n\t" \
        ".insn i 0x0b, 0b011, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b011, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b011, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b011, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        : "r+" (wt), "r+" (addr) + \
    ); \
})

#define matmul8_b0(wt,addr) ({ \
    __asm__ __volatile__ ( \
        ".insn i 0x0b, 0b000, %0, %0, 64\n\t" \
        ".insn i 0x0b, 0b010, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b010, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b010, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b010, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b010, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b010, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b010, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b010, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        : "r+" (wt), "r+" (addr) : \
    ); \
})

#define matmul8_b1(wt,addr) ({ \
    __asm__ __volatile__ ( \
        ".insn i 0x0b, 0b001, %0, %0, 64\n\t" \
        ".insn i 0x0b, 0b011, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b011, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b011, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b011, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b011, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b011, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b011, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        ".insn i 0x0b, 0b011, %1, %1, 64\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        "nop\n\t" \
        : "r+" (wt), "r+" (addr) : \
    ); \
})

int main(int argc, char** argv) {
    for (int t = 0; t < 2; t++) {

        uint64_t start_cycles = read_csr(mcycle);

        uint64_t A_base = (uint64_t)(&A);
        uint64_t W_base = (uint64_t)(&W);
        uint64_t R_base = (uint64_t)(&result);

        A_base -= 16;
        W_base -= 16;

        bool buf = 0;

        #if defined(USE_TC)
            for (uint64_t i = 0; i < I_PRIME; i++) {
                for (uint64_t j = 0; j < J_PRIME; j++) {
                    for (uint64_t k = 0; k < K_PRIME; k++) {
                        uint64_t offset_A = (j + i*J_PRIME) ;
                        uint64_t offset_W = (k + j*K_PRIME) ;
                        uint64_t offset_R = (k + i*K_PRIME) ;

                        //bp_printf("%ld, %ld -> %ld\n", offset_A, offset_W, offset_R);

                        uint64_t A_ptr = A_base + (64 * offset_A);
                        uint64_t W_ptr = W_base + (64 * offset_W);
                        uint64_t R_ptr = R_base + (64 * offset_R);

                        if (!buf) {
                            nop();
                            nop();
                            nop();
                            tensor_csr_st(0, R_ptr);
                            matmul1_b0(W_ptr, A_ptr);
                        } else {
                            nop();
                            nop();
                            nop();
                            tensor_csr_st(1, R_ptr);
                            matmul1_b1(W_ptr, A_ptr);
                        }

                        buf = !buf;
            }}}

        //#elif defined(USE_FOR_LOOP)
        //    for (int i = 0; i < I; i++) {
        //    for (int j = 0; j < J; j++) {
        //    for (int k = 0; k < K; k++) {
        //        uint32_t a_idx = j + i*J;
        //        uint32_t w_idx = k + j*K;
        //        uint32_t p_idx = k + i*K;
        //        result[p_idx] ^= A[a_idx] ^ W[w_idx];
        //    }}}

        #endif

        uint64_t end_cycles = read_csr(mcycle);
        uint64_t diff = end_cycles - start_cycles;
        bp_printf("Cycle Count = %lu\n", diff);

        float flops = (float)(I*J*K) / diff;
        bp_printf("FLOPS = %f\n", flops);

        uint64_t errors = 0;
        for (int i = 0; i < I*K; i++) {
            if (result[i] != expected[i]) {
                errors++;
                bp_printf("%d: %d != %d\n", i, result[i], expected[i]);
            }
        }

        bp_printf("Number of errors = %ld\n", errors);
    } // for (t)

    return 0;
}