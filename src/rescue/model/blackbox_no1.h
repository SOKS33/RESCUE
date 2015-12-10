/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
// Copied from Sebastian Kühlmorgen TU Dresden, original Matlab code from Petri Komulainen UOULU

#ifndef BLACKBOX_NO_1_H
#define BLACKBOX_NO_1_H

#include "ns3/object.h"
#include <cmath>
#include <numeric>

namespace ns3 {

    class OutputBlackbox_no1 {
    public:
        double m_ber; // bit error probability
        double m_mi; // Equivalent mutual information
        double m_p_floor; // error floor
    };

    const double capacity_table[] = {0.000228074055137, 0.000255898363145, 0.000287116472577, 0.000322142159166, 0.000361439601359, 0.000405529508247, 0.000454995989498,
        0.000510494256346, 0.000572759253196, 0.000642615331042, 0.000720987086898, 0.000808911507812, 0.000907551574003, 0.001018211493327,
        0.001142353758787, 0.001281618242307, 0.001437843561665, 0.001613090983410, 0.001809671152934, 0.002030173973783, 0.002277501991710,
        0.002554907675103, 0.002866035022045, 0.003214965965371, 0.003606272090353, 0.004045072224639, 0.004537096506248, 0.005088757581712,
        0.005707229631776, 0.006400535964624, 0.007177645954175, 0.008048582130704, 0.009024538249195, 0.010118009162570, 0.011342933306559,
        0.012714848552914, 0.014251062099122, 0.015970834924715, 0.017895581143755, 0.020049082304606, 0.022457716313831, 0.025150700170403,
        0.028160345066511, 0.031522321617007, 0.035275931994705, 0.039464384546879, 0.044135065025023, 0.049339796855346, 0.055135080900667,
        0.061582302918103, 0.068747894425013, 0.076703430001293, 0.085525641272558, 0.095296325081324, 0.106102120873068, 0.118034130391106,
        0.131187351770214, 0.145659900524608, 0.161551992320432, 0.178964667468841, 0.197998245481067, 0.218750510475091, 0.241314645260690,
        0.265776953796411, 0.292214438116783, 0.320692325662844, 0.351261673973004, 0.383957208239949, 0.418795568063670, 0.455774146133212,
        0.494870686065129, 0.536043762374013, 0.579234188727825, 0.624367293657466, 0.671355877731348, 0.720103546293603, 0.770508029758063,
        0.822464091324533, 0.875865693527003, 0.930607222865253, 0.986583673923147, 1.043689651754715, 1.101816774912869, 1.160848599289729,
        1.220651805093632, 1.281062550232012, 1.341868001279440, 1.402785112655803, 1.463441090115595, 1.523361497110288, 1.581971594025355,
        1.638614025290232, 1.692582292041243, 1.743166100274570, 1.789702839513347, 1.831629259507196, 1.868528094853573, 1.900165048298803,
        1.926511905528896, 1.947752042218393, 1.964265850385905, 1.976595995851948, 1.985395701162194, 1.991366695035316, 1.995196047621912,
        1.997501812796087, 1.998795647958647, 1.999466600421375, 1.999785162804531, 1.999922201742042, 1.999974992360615, 1.999992966438974,
        1.999998296783124, 1.999999651268849, 1.999999940842711, 1.999999991873342, 1.999999999118798, 1.999999999926724, 1.999999999995483,
        1.999999999999811, 2.000000000000000}; // length: 121

    class BlackBox_no1 : public Object {
    public:
        static TypeId GetTypeId(void);

        BlackBox_no1();
        virtual ~BlackBox_no1();

        /*
         * Calculate mutual information (MI) metric and bit error probability (p) at a receiving node
         * - combined over multiple received copies (different coding for each, but same code rate)
         * - constellation constrained MI used
         * - normalized with spectral efficiency (information bit level)
         * - in theory MI per information bit is perfect when it is unity, but the metric here can get higher values
         * - bit error probability calculated based on rate-distortion theory, and further lower bounded by error-floor calculation (as derived by TUD)

         * References
         * - D2.1.1 Interim Report of Detailed Coding/Decoding Algorithms and Correlation Estimation Techniques
         * - Xiaobo Zhou, Meng Cheng, Xin He, and Tad Matsumoto. "Exact and Approximated Outage Probability Analyses
         * for Decode-and-Forward Relaying System Allowing Intra-link Errors". In: IEEE Trans. Wireless Commun. 2014.
         *
         * Inputs [dimensions]
         * - SNR_dB [num_links] SNR (instantaneous Es/N0 at the receiver side, includes the channel fading state) per complex symbol for each link in dB
         * - linkPER [num_links] packet error probability at each transmitter, range 0.0...0.5
         * - constellation_size [scalar] 2=BPSK, 4=QPSK, 16=16QAM
         * - spectral_efficiency [scalar] number of information bits per complex symbol (affected by code rate and constellation size)
         *   used for normalization purposes only
         *
         * Calculate the bit error rate. Every link has its own constellationSize and spectralEfficiency
         */
        static void CalculateRescueBitErrorRate(std::vector<double> snr_db, std::vector<double> linkBitER, std::vector<int> constellationSize, std::vector<double> spectralEfficiency, OutputBlackbox_no1 & outbl_no1);

        /*
         *  Calculate the bit error rate
         *  outbl_no1 = {ber, mi, p_floor}
         */
        static void CalculateRescueBitErrorRate(std::vector<double> snr_db, std::vector<double> linkBitER, int constellationSize, double spectralEfficiency, OutputBlackbox_no1 & outbl_no1);

        /*
         *  Calculate the bit error rate
         *  outbl_no1 = {ber, mi, p_floor}
         */
        static double CalculateRescueBitErrorRate(std::vector<double> snr_db, std::vector<double> linkBitER, int constellationSize, double spectralEfficiency);

        /*
         *  Aggregate the bit error probabilities ber* = ber1*(1-ber2) + (1-ber1)*ber2
         */
        static double BinaryConvolution(double ber1, double ber2);

    private:
        /*
         * Lookup for constellationSize 2 and 4
         */
        static double J_Function(double sigma);

        /*
         * Read the four-PAM-capacity table for a given SNR
         * Table includes capacity values for SNRs from -35 to 25 dB with 0.5dB granularity
         *
         * Inputs [dimensions]
         * - snr_dB [num_links] SNR (instantaneous Es/N0 at the receiver side, includes the channel fading state) per complex symbol for each link in dB
         *
         * Outputs [dimensions]
         * - c [num_links] four-PAM-capacity for each link
         */
        static double Four_Pam_Capacity_Table_Lookup(double snr_DB);

        /*
         * Inverse of the (increasing) binary entropy function
         * H(p) = -p*log2(p) - (1-p)*log2(1-p), where p = 0.0...0.5
         * Input x should be scalar and between 0...1 (inputs outside range truncated back to range)
         * Algorithm: find p such that x-H(p)=0 via bisection.
         * Output p is in the range from 0.0 to 0.5
         */
        static double InvH(double x);

        /*
         * Calculate error floor at receiving node
         * - Based on bit error probabilities at parallel transmitters (relays)
         * - Formula derived by TUD
         * - Error floor equals to zero if there is at least one zero TX error probability

         * Inputs [dimensions]
         * - p_tx [num_links] bit error probability at each transmitter, range 0.0...0.5

         * Outputs [dimensions]
         * - p_floor [scalar] error probability at destination assuming infinite SNR for all links
         */
        static double ErrorFloor(std::vector<double> p_tx);

        /*
         * Helper function to create the bit divisor vector
         */
        static std::vector<double> GenerateBitDivisor(int numberLinks); //(1, 2, 4, 8, 16, ...)

    };

}

#endif /* BLACKBOX_NO_1_H */
