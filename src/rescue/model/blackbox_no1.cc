/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "blackbox_no1.h"

NS_LOG_COMPONENT_DEFINE("BlackBox");

namespace ns3 {

    NS_OBJECT_ENSURE_REGISTERED(BlackBox_no1);

    TypeId
    BlackBox_no1::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::BlackBox_no1")
                .SetParent<Object>()
                .AddConstructor<BlackBox_no1>();
        return tid;
    }

    BlackBox_no1::BlackBox_no1() {
    }

    BlackBox_no1::~BlackBox_no1() {
    }

    void
    BlackBox_no1::CalculateRescueBitErrorRate(std::vector<double> snr_db, std::vector<double> linkBitER, std::vector<int> constellationSize, std::vector<double> spectralEfficiency, OutputBlackbox_no1 & outbl_no1) {
        NS_ASSERT(snr_db.size() == linkBitER.size());
        NS_ASSERT(snr_db.size() == constellationSize.size());
        NS_ASSERT(snr_db.size() == spectralEfficiency.size());

        int num_links = snr_db.size(); // number of incoming links

        std::vector<double> esN0;
        for (std::vector<double>::iterator it = snr_db.begin(); it != snr_db.end(); ++it) {
            esN0.push_back(std::pow(10, *it / 10)); // convert SNR to linear scale
        }

        std::vector<double> C;
        for (uint32_t i = 0; i < esN0.size(); ++i) {
            // Determine constellation constrained capacity per complex symbol
            if (constellationSize[i] == 2) // BPSK
                C.push_back(BlackBox_no1::J_Function(std::sqrt(8 * esN0[i])));
            else if (constellationSize[i] == 4) // QPSK
                C.push_back(2 * BlackBox_no1::J_Function(std::sqrt(4 * esN0[i])));
            else if (constellationSize[i] == 16) // 16QAM
                C.push_back(Four_Pam_Capacity_Table_Lookup(snr_db[i])); // the multiplication by 2 take place in the function
            else {
                std::cout << "Unsupported constellation size." << std::endl;
            }
        }

        // Phi = constellation constrained mutual information per information bit i.e. channel capacity normalized with spectral efficiency
        // Note that this can be >1
        std::vector<double> Phi;
        for (uint32_t i = 0; i < C.size(); ++i) {
            Phi.push_back(C[i] / spectralEfficiency[i]);
        }

        double H; // binary entropy function
        std::vector<double> MI;

        // Calculate mutual information per transmitter, and the sum
        for (int i = 0; i < num_links; i++) {
            if (linkBitER[i] > 0) {
                H = -(1 - linkBitER[i]) * log2(1 - linkBitER[i]) - linkBitER[i] * log2(linkBitER[i]);
                if (linkBitER.size() == 1)
                    MI.push_back((1 - H) * Phi[i]);
                else
                    MI.push_back(std::min((1 - H) * Phi[i], 1 - H)); // MI increment always < 1
            } else
                MI.push_back(Phi[i]); // if no errors in transmitter, allow MI increment > 1
        }

        double MI_tot = std::accumulate(MI.begin(), MI.end(), 0.0);

        // Determine expected bit error probability after joint decoding at the receiver
        double p_rateDistortion = BlackBox_no1::InvH(1 - std::min(MI_tot, 1.0));
        double p_floor = BlackBox_no1::ErrorFloor(linkBitER);
        double p_rx = std::max(p_rateDistortion, p_floor); // bit error rate

        outbl_no1.m_ber = p_rx;
        outbl_no1.m_mi = MI_tot;
        outbl_no1.m_p_floor = p_floor;
    }

    void
    BlackBox_no1::CalculateRescueBitErrorRate(std::vector<double> linkSnr_db, std::vector<double> linkBitER, int constellationSize, double spectralEfficiency, OutputBlackbox_no1 & outbl_no1) {
        NS_ASSERT(linkSnr_db.size() == linkBitER.size());

        std::vector<int> linkConstellationSize;
        std::vector<double> linkSpectralEfficiency;

        for (uint32_t i = 0; i < linkSnr_db.size(); ++i) {
            linkConstellationSize.push_back(constellationSize);
            linkSpectralEfficiency.push_back(spectralEfficiency);
        }

        BlackBox_no1::CalculateRescueBitErrorRate(linkSnr_db, linkBitER, linkConstellationSize, linkSpectralEfficiency, outbl_no1);
    }

    double
    BlackBox_no1::CalculateRescueBitErrorRate(std::vector<double> snr_db, std::vector<double> linkBitER, int constellationSize, double spectralEfficiency) {
        OutputBlackbox_no1 outputBlackbox_no1;
        BlackBox_no1::CalculateRescueBitErrorRate(snr_db, linkBitER, constellationSize, spectralEfficiency, outputBlackbox_no1);

        return outputBlackbox_no1.m_ber;
    }

    double
    BlackBox_no1::J_Function(double sigma) {
        double x = 0;
        double y = 0;

        const double H1 = 0.3073;
        const double H2 = 0.8935;
        const double H3 = 1.1064;

        if (sigma >= 0) {
            x = -H1 * std::pow(sigma, 2 * H2);
            y = 1 - std::pow(2, x);
        }

        return std::pow(y, H3);
    }

    double
    BlackBox_no1::Four_Pam_Capacity_Table_Lookup(double snr_DB) {
        double C;
        double snr_lower_dB;
        int lower_ind;
        double d;

        if (snr_DB < -35.0) // -infinity < snr < -35
            C = 0.0;
        else if (snr_DB >= 25) // 25 <= snr < infinity
            C = 2.0;
        else {
            snr_lower_dB = 0.5 * std::floor(2 * snr_DB); // with 0.5 dB granularity
            lower_ind = round(2 * (snr_lower_dB + 35) + 1); // lower index in table
            d = (snr_DB - snr_lower_dB) / 0.5; // normalized distance from lower limit (always non-negative)
            C = 2 * ((1 - d) * capacity_table[lower_ind - 1] + d * capacity_table[lower_ind]);
        }

        return C;
    }

    double
    BlackBox_no1::InvH(double x) {
        double p;

        if (x <= 0 || std::abs(x - 0) <= 1e-6)
            p = 0;
        else if (x > 1)
            p = 0.5;
        else // 0 < x <= 1
        {
            p = 0.5; // start from the end of search area 0.0 ... 0.5
            double delta = x - 1; // x - H(0.5)
            double step = 0.25; // distance to the next middle point

            while (std::abs(delta) > 1e-6) {
                p = p + ((delta > 0) - (delta < 0)) * step; // sign function = (delta > 0) - (delta < 0)
                delta = x + p * log2(p) + (1 - p) * log2(1 - p); // x-H(p)
                step = 0.5 * step;
            }
        }

        return p;
    }

    double
    BlackBox_no1::ErrorFloor(std::vector<double> p_tx) {
        int numLinks = p_tx.size();
        bool zeroErrorLink = false;
        double p_floor = 0;
        std::vector<double> bitDivisor;
        std::vector<double> Ls;

        // if there is at least one zero link error probability, error floor disappears
        for (std::vector<double>::iterator it = p_tx.begin(); it != p_tx.end(); ++it) {
            if (*it == 0) {
                zeroErrorLink = true;
                break;
            }
        }

        if (zeroErrorLink)
            p_floor = 0;
        else {
            bitDivisor = BlackBox_no1::GenerateBitDivisor(numLinks); // (1, 2, 4, 8, 16, ...)

            for (std::vector<double>::iterator it = p_tx.begin(); it != p_tx.end(); ++it) {
                Ls.push_back(std::log((1 - *it) / *it)); // non-negative numbers, LLRs per link after weighting by f_c-function
            }

            // go through all error event combinations (combination==0 means that all links are correct)
            p_floor = 0;

            for (int combination = 0; combination < std::pow(2.0, numLinks) - 1; ++combination) // each bit represents state of one link
            {
                // calculate the probability of the combination and the corresponding soft decision
                double p_combination = 1;
                double L = 0;

                for (int i = 0; i < numLinks; ++i) {
                    // find the bit (0 or 1) corresponding to link 'i' (just bit shift to right and mask the right-most bit)
                    int bit = (int) std::floor(static_cast<double> (combination) / bitDivisor[i]) % 2;

                    if (bit == 1) // no error
                    {
                        p_combination = p_combination * (1 - p_tx[i]);
                        L = L + Ls[i];
                    } else // error in link
                    {
                        p_combination = p_combination * p_tx[i];
                        L = L - Ls[i];
                    }
                } // links loop

                // update error floor if the decision is wrong or uncertain
                if (L < 0)
                    p_floor = p_floor + p_combination;
                else if (L == 0)
                    p_floor = p_floor + p_combination / 2;

            } // combination loop

        } // if zeroErrorLink

        return p_floor;
    }

    std::vector<double>
    BlackBox_no1::GenerateBitDivisor(int numberLinks) {
        std::vector<double> bitDivisor;

        for (int i = 0; i < numberLinks; i++) {
            bitDivisor.push_back(std::pow(2.0, static_cast<double> (i)));
        }

        return bitDivisor;
    }

    double
    BlackBox_no1::BinaryConvolution(double ber1, double ber2) {
        double ber(0);

        if (ber1 < 0 || ber2 < 0 || ber1 > 1 || ber2 > 1) {
            ber = -1;
            NS_FATAL_ERROR("Error probabilities are negative or greater than 1.");
        }

        ber = ber1 * (1 - ber2) + (1 - ber1) * ber2;

        return ber;
    }

} // namespace ns3

