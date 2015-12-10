#include "ns3/log.h"
#include "blackbox_no2.h"

#include <vector>
#include <algorithm>

NS_LOG_COMPONENT_DEFINE("BlackBox_no2");

namespace ns3 {

    NS_OBJECT_ENSURE_REGISTERED(BlackBox_no2);

    TypeId
    BlackBox_no2::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::BlackBox_no2")
                .SetParent<Object>()
                .AddConstructor<BlackBox_no2>();
        return tid;
    }

    BlackBox_no2::BlackBox_no2() {
    }

    BlackBox_no2::~BlackBox_no2() {
    }

    int
    BlackBox_no2::CalculateRescueBitErrorNumber(double MI, double p_floor, int n_link, int constellationSize, int block_length) {
        NS_ASSERT(n_link < 4 || n_link > 10); // >10 for mixed modulations

        std::vector<double> mi_tab;
        std::vector<double> alpha_tab;
        std::vector<double> mu_tab;
        std::vector<double> sigma_tab;
        std::vector<double> ber_tab;
        std::vector<double> ber_tabLog;

        switch (constellationSize) {
            case 2:
                switch (n_link) {
                    case 1:
                        mi_tab.insert(mi_tab.begin(), BPSK_1_mi, BPSK_1_mi + sizeof (BPSK_1_mi) / sizeof (double));
                        alpha_tab.insert(alpha_tab.begin(), BPSK_1_alpha, BPSK_1_alpha + sizeof (BPSK_1_alpha) / sizeof (double));
                        mu_tab.insert(mu_tab.begin(), BPSK_1_mu, BPSK_1_mu + sizeof (BPSK_1_mu) / sizeof (double));
                        sigma_tab.insert(sigma_tab.begin(), BPSK_1_sigma, BPSK_1_sigma + sizeof (BPSK_1_sigma) / sizeof (double));
                        ber_tab.insert(ber_tab.begin(), BPSK_1_ber, BPSK_1_ber + sizeof (BPSK_1_ber) / sizeof (double));
                        ber_tabLog.insert(ber_tabLog.begin(), BPSK_1_ber_Log, BPSK_1_ber_Log + sizeof (BPSK_1_ber_Log) / sizeof (double));
                        break;
                    case 2:
                        mi_tab.insert(mi_tab.begin(), BPSK_2_mi, BPSK_2_mi + sizeof (BPSK_2_mi) / sizeof (double));
                        alpha_tab.insert(alpha_tab.begin(), BPSK_2_alpha, BPSK_2_alpha + sizeof (BPSK_2_alpha) / sizeof (double));
                        mu_tab.insert(mu_tab.begin(), BPSK_2_mu, BPSK_2_mu + sizeof (BPSK_2_mu) / sizeof (double));
                        sigma_tab.insert(sigma_tab.begin(), BPSK_2_sigma, BPSK_2_sigma + sizeof (BPSK_2_sigma) / sizeof (double));
                        ber_tab.insert(ber_tab.begin(), BPSK_2_ber, BPSK_2_ber + sizeof (BPSK_2_ber) / sizeof (double));
                        ber_tabLog.insert(ber_tabLog.begin(), BPSK_2_ber_Log, BPSK_2_ber_Log + sizeof (BPSK_2_ber_Log) / sizeof (double));
                        break;
                    case 3:
                        mi_tab.insert(mi_tab.begin(), BPSK_3_mi, BPSK_3_mi + sizeof (BPSK_3_mi) / sizeof (double));
                        alpha_tab.insert(alpha_tab.begin(), BPSK_3_alpha, BPSK_3_alpha + sizeof (BPSK_3_alpha) / sizeof (double));
                        mu_tab.insert(mu_tab.begin(), BPSK_3_mu, BPSK_3_mu + sizeof (BPSK_3_mu) / sizeof (double));
                        sigma_tab.insert(sigma_tab.begin(), BPSK_3_sigma, BPSK_3_sigma + sizeof (BPSK_3_sigma) / sizeof (double));
                        ber_tab.insert(ber_tab.begin(), BPSK_3_ber, BPSK_3_ber + sizeof (BPSK_3_ber) / sizeof (double));
                        ber_tabLog.insert(ber_tabLog.begin(), BPSK_3_ber_Log, BPSK_3_ber_Log + sizeof (BPSK_3_ber_Log) / sizeof (double));
                        break;
                    default:
                        NS_FATAL_ERROR("Error: Number of parallel links at maximum 3.");
                        return -1;
                }
                ;
                break;
            case 4:
                switch (n_link) {
                    case 1:
                        mi_tab.insert(mi_tab.begin(), QPSK_1_mi, QPSK_1_mi + sizeof (QPSK_1_mi) / sizeof (double));
                        alpha_tab.insert(alpha_tab.begin(), QPSK_1_alpha, QPSK_1_alpha + sizeof (QPSK_1_alpha) / sizeof (double));
                        mu_tab.insert(mu_tab.begin(), QPSK_1_mu, QPSK_1_mu + sizeof (QPSK_1_mu) / sizeof (double));
                        sigma_tab.insert(sigma_tab.begin(), QPSK_1_sigma, QPSK_1_sigma + sizeof (QPSK_1_sigma) / sizeof (double));
                        ber_tab.insert(ber_tab.begin(), QPSK_1_ber, QPSK_1_ber + sizeof (QPSK_1_ber) / sizeof (double));
                        ber_tabLog.insert(ber_tabLog.begin(), QPSK_1_ber_Log, QPSK_1_ber_Log + sizeof (QPSK_1_ber_Log) / sizeof (double));
                        break;
                    case 2:
                        mi_tab.insert(mi_tab.begin(), QPSK_2_mi, QPSK_2_mi + sizeof (QPSK_2_mi) / sizeof (double));
                        alpha_tab.insert(alpha_tab.begin(), QPSK_2_alpha, QPSK_2_alpha + sizeof (QPSK_2_alpha) / sizeof (double));
                        mu_tab.insert(mu_tab.begin(), QPSK_2_mu, QPSK_2_mu + sizeof (QPSK_2_mu) / sizeof (double));
                        sigma_tab.insert(sigma_tab.begin(), QPSK_2_sigma, QPSK_2_sigma + sizeof (QPSK_2_sigma) / sizeof (double));
                        ber_tab.insert(ber_tab.begin(), QPSK_2_ber, QPSK_2_ber + sizeof (QPSK_2_ber) / sizeof (double));
                        ber_tabLog.insert(ber_tabLog.begin(), QPSK_2_ber_Log, QPSK_2_ber_Log + sizeof (QPSK_2_ber_Log) / sizeof (double));
                        break;
                    case 3:
                        mi_tab.insert(mi_tab.begin(), QPSK_3_mi, QPSK_3_mi + sizeof (QPSK_3_mi) / sizeof (double));
                        alpha_tab.insert(alpha_tab.begin(), QPSK_3_alpha, QPSK_3_alpha + sizeof (QPSK_3_alpha) / sizeof (double));
                        mu_tab.insert(mu_tab.begin(), QPSK_3_mu, QPSK_3_mu + sizeof (QPSK_3_mu) / sizeof (double));
                        sigma_tab.insert(sigma_tab.begin(), QPSK_3_sigma, QPSK_3_sigma + sizeof (QPSK_3_sigma) / sizeof (double));
                        ber_tab.insert(ber_tab.begin(), QPSK_3_ber, QPSK_3_ber + sizeof (QPSK_3_ber) / sizeof (double));
                        ber_tabLog.insert(ber_tabLog.begin(), QPSK_3_ber_Log, QPSK_3_ber_Log + sizeof (QPSK_3_ber_Log) / sizeof (double));
                        break;
                    default:
                        NS_FATAL_ERROR("Error: Number of parallel links at maximum 3.");
                        return -1;
                }
                ;
                break;
            case 16:
                switch (n_link) {
                    case 1:
                        mi_tab.insert(mi_tab.begin(), QAM16_1_mi, QAM16_1_mi + sizeof (QAM16_1_mi) / sizeof (double));
                        alpha_tab.insert(alpha_tab.begin(), QAM16_1_alpha, QAM16_1_alpha + sizeof (QAM16_1_alpha) / sizeof (double));
                        mu_tab.insert(mu_tab.begin(), QAM16_1_mu, QAM16_1_mu + sizeof (QAM16_1_mu) / sizeof (double));
                        sigma_tab.insert(sigma_tab.begin(), QAM16_1_sigma, QAM16_1_sigma + sizeof (QAM16_1_sigma) / sizeof (double));
                        ber_tab.insert(ber_tab.begin(), QAM16_1_ber, QAM16_1_ber + sizeof (QAM16_1_ber) / sizeof (double));
                        ber_tabLog.insert(ber_tabLog.begin(), QAM16_1_ber_Log, QAM16_1_ber_Log + sizeof (QAM16_1_ber_Log) / sizeof (double));
                        break;
                    case 2:
                        mi_tab.insert(mi_tab.begin(), QAM16_2_mi, QAM16_2_mi + sizeof (QAM16_2_mi) / sizeof (double));
                        alpha_tab.insert(alpha_tab.begin(), QAM16_2_alpha, QAM16_2_alpha + sizeof (QAM16_2_alpha) / sizeof (double));
                        mu_tab.insert(mu_tab.begin(), QAM16_2_mu, QAM16_2_mu + sizeof (QAM16_2_mu) / sizeof (double));
                        sigma_tab.insert(sigma_tab.begin(), QAM16_2_sigma, QAM16_2_sigma + sizeof (QAM16_2_sigma) / sizeof (double));
                        ber_tab.insert(ber_tab.begin(), QAM16_2_ber, QAM16_2_ber + sizeof (QAM16_2_ber) / sizeof (double));
                        ber_tabLog.insert(ber_tabLog.begin(), QAM16_2_ber_Log, QAM16_2_ber_Log + sizeof (QAM16_2_ber_Log) / sizeof (double));
                        break;
                    case 3:
                        mi_tab.insert(mi_tab.begin(), QAM16_3_mi, QAM16_3_mi + sizeof (QAM16_3_mi) / sizeof (double));
                        alpha_tab.insert(alpha_tab.begin(), QAM16_3_alpha, QAM16_3_alpha + sizeof (QAM16_3_alpha) / sizeof (double));
                        mu_tab.insert(mu_tab.begin(), QAM16_3_mu, QAM16_3_mu + sizeof (QAM16_3_mu) / sizeof (double));
                        sigma_tab.insert(sigma_tab.begin(), QAM16_3_sigma, QAM16_3_sigma + sizeof (QAM16_3_sigma) / sizeof (double));
                        ber_tab.insert(ber_tab.begin(), QAM16_3_ber, QAM16_3_ber + sizeof (QAM16_3_ber) / sizeof (double));
                        ber_tabLog.insert(ber_tabLog.begin(), QAM16_3_ber_Log, QAM16_3_ber_Log + sizeof (QAM16_3_ber_Log) / sizeof (double));
                        break;
                    default:
                        NS_FATAL_ERROR("Error: Number of parallel links at maximum 3.");
                        return -1;
                }
                ;
                break;
            case 416: // constellationSize 4 - QPSK, 16 - 16QAM
                switch (n_link) {
                    case 11: // 1 x QPSK, 1 x 16QAM
                        mi_tab.insert(mi_tab.begin(), QPSK_QAM16_2_mi, QPSK_QAM16_2_mi + sizeof (QPSK_QAM16_2_mi) / sizeof (double));
                        alpha_tab.insert(alpha_tab.begin(), QPSK_QAM16_2_alpha, QPSK_QAM16_2_alpha + sizeof (QPSK_QAM16_2_alpha) / sizeof (double));
                        mu_tab.insert(mu_tab.begin(), QPSK_QAM16_2_mu, QPSK_QAM16_2_mu + sizeof (QPSK_QAM16_2_mu) / sizeof (double));
                        sigma_tab.insert(sigma_tab.begin(), QPSK_QAM16_2_sigma, QPSK_QAM16_2_sigma + sizeof (QPSK_QAM16_2_sigma) / sizeof (double));
                        ber_tab.insert(ber_tab.begin(), QPSK_QAM16_2_ber, QPSK_QAM16_2_ber + sizeof (QPSK_QAM16_2_ber) / sizeof (double));
                        ber_tabLog.insert(ber_tabLog.begin(), QPSK_QAM16_2_ber_Log, QPSK_QAM16_2_ber_Log + sizeof (QPSK_QAM16_2_ber_Log) / sizeof (double));
                        break;
                    case 21: // 2 x QPSK, 1 x 16QAM
                        mi_tab.insert(mi_tab.begin(), QPSK_QPSK_QAM16_3_mi, QPSK_QPSK_QAM16_3_mi + sizeof (QPSK_QPSK_QAM16_3_mi) / sizeof (double));
                        alpha_tab.insert(alpha_tab.begin(), QPSK_QPSK_QAM16_3_alpha, QPSK_QPSK_QAM16_3_alpha + sizeof (QPSK_QPSK_QAM16_3_alpha) / sizeof (double));
                        mu_tab.insert(mu_tab.begin(), QPSK_QPSK_QAM16_3_mu, QPSK_QPSK_QAM16_3_mu + sizeof (QPSK_QPSK_QAM16_3_mu) / sizeof (double));
                        sigma_tab.insert(sigma_tab.begin(), QPSK_QPSK_QAM16_3_sigma, QPSK_QPSK_QAM16_3_sigma + sizeof (QPSK_QPSK_QAM16_3_sigma) / sizeof (double));
                        ber_tab.insert(ber_tab.begin(), QPSK_QPSK_QAM16_3_ber, QPSK_QPSK_QAM16_3_ber + sizeof (QPSK_QPSK_QAM16_3_ber) / sizeof (double));
                        ber_tabLog.insert(ber_tabLog.begin(), QPSK_QPSK_QAM16_3_ber_Log, QPSK_QPSK_QAM16_3_ber_Log + sizeof (QPSK_QPSK_QAM16_3_ber_Log) / sizeof (double));
                        break;
                    case 12: // 1 x QPSK, 2 x 16QAM
                        mi_tab.insert(mi_tab.begin(), QPSK_QAM16_QAM16_3_mi, QPSK_QAM16_QAM16_3_mi + sizeof (QPSK_QAM16_QAM16_3_mi) / sizeof (double));
                        alpha_tab.insert(alpha_tab.begin(), QPSK_QAM16_QAM16_3_alpha, QPSK_QAM16_QAM16_3_alpha + sizeof (QPSK_QAM16_QAM16_3_alpha) / sizeof (double));
                        mu_tab.insert(mu_tab.begin(), QPSK_QAM16_QAM16_3_mu, QPSK_QAM16_QAM16_3_mu + sizeof (QPSK_QAM16_QAM16_3_mu) / sizeof (double));
                        sigma_tab.insert(sigma_tab.begin(), QPSK_QAM16_QAM16_3_sigma, QPSK_QAM16_QAM16_3_sigma + sizeof (QPSK_QAM16_QAM16_3_sigma) / sizeof (double));
                        ber_tab.insert(ber_tab.begin(), QPSK_QAM16_QAM16_3_ber, QPSK_QAM16_QAM16_3_ber + sizeof (QPSK_QAM16_QAM16_3_ber) / sizeof (double));
                        ber_tabLog.insert(ber_tabLog.begin(), QPSK_QAM16_QAM16_3_ber_Log, QPSK_QAM16_QAM16_3_ber_Log + sizeof (QPSK_QAM16_QAM16_3_ber_Log) / sizeof (double));
                        break;
                    default:
                        NS_FATAL_ERROR("Error: Wrong constellation size in mixed modulation.");
                        return -1;
                }
                ;
                break;
            default:
                NS_FATAL_ERROR("Error: Constellation size is not supported in this version.\n Please choose the constellation size: 2 - BPSK, 4 - QPSK, 16 - 16QAM \n");
                return -1;
        }

        // bound MI to the table entries
        MI = std::max(MI, mi_tab[0]);
        MI = std::min(MI, mi_tab[mi_tab.size() - 1]);

        // Get the three parameters (alpha, mu, sigma) of the BE distribution by linear interpolation
        double alpha = BlackBox_no2::Interpolation(mi_tab, alpha_tab, MI);
        double mu(0);
        double sigma(0);

        if (constellationSize >= 16) {
            mu = BlackBox_no2::Interpolation(mi_tab, mu_tab, MI) * block_length / 8400;
            sigma = BlackBox_no2::Interpolation(mi_tab, sigma_tab, MI) * block_length / 8400;
        } else {
            mu = BlackBox_no2::Interpolation(mi_tab, mu_tab, MI) * block_length / 8000;
            sigma = BlackBox_no2::Interpolation(mi_tab, sigma_tab, MI) * block_length / 8000;
        }

        // Generate error count according to the BE distribution
        Ptr<UniformRandomVariable> random_unif = CreateObject<UniformRandomVariable>();
        Ptr<NormalRandomVariable> random_norm = CreateObject<NormalRandomVariable>();
        random_norm->SetAttribute("Mean", DoubleValue(mu));
        random_norm->SetAttribute("Variance", DoubleValue(sigma * sigma));

        int n_errors(-1);

        if (random_unif->GetValue() > alpha) {
            n_errors = round(random_norm->GetValue());

            if (n_errors < 0)
                n_errors = 0;
        } else
            n_errors = 0;

        //Generate errors that are evenly distributed
        if (p_floor > 0) {
            std::vector<double> e = BlackBox_no2::GenerateRandomBlock(block_length, p_floor);
            n_errors += BlackBox_no2::FindNumberErrors(e);
        }

        return n_errors;
    }

    std::vector<double>
    BlackBox_no2::GenerateRandomBlock(int blockLength, double p_floor) {
        std::vector<double> block;
        Ptr<UniformRandomVariable> randomUnif = CreateObject<UniformRandomVariable>();

        for (int i = 0; i < blockLength; ++i) {
            block.push_back(((double) BlackBox_no2::sgn(randomUnif->GetValue() - 1.0 + p_floor) + 1.0) / 2.0);
        }

        return block;
    }

    int
    BlackBox_no2::FindNumberErrors(std::vector<double> block) {
        int counter(0);

        for (uint32_t i = 0; i < block.size(); ++i) {
            if (block[i] != 0)
                counter++;
        }

        return counter;
    }

    inline int
    BlackBox_no2::sgn(double x) {
        if (x == 0)
            return 0;
        else
            return (x > 0) ? 1 : -1;
    }

    bool
    sortByBER(const BlackBox_no2Entry &lhs, const BlackBox_no2Entry &rhs) { // helper function to sort the vector m_rescueCBFBuffer by UID
        return lhs.m_ber < rhs.m_ber;
    }

    int
    BlackBox_no2::CalculateRescueBitErrorNumber(std::vector<double> & snr_db, std::vector<double> linkBitER, int constellationSize, double spectralEfficiency, int block_length) {
        NS_ASSERT(snr_db.size() == linkBitER.size());

        std::vector<BlackBox_no2Entry> arrangeBER; // use the packets with the best ber

        if (snr_db.size() > 3) // the lookup table offers just values for three parallel links
        {
            for (uint32_t i = 0; i < snr_db.size(); ++i) {
                arrangeBER.push_back(BlackBox_no2Entry(snr_db[i], linkBitER[i], constellationSize, spectralEfficiency)); // fill vector arrangeBER with all links
            }

            std::sort(arrangeBER.begin(), arrangeBER.end(), sortByBER); // sort according the best ber

            snr_db.clear();
            linkBitER.clear();

            for (uint32_t i = 0; i < 3; ++i) // refill snr_db, linkBitER with the best ber
            {
                snr_db.push_back(arrangeBER[i].m_snr);
                linkBitER.push_back(arrangeBER[i].m_ber);
            }
        }

        OutputBlackbox_no1 outputBlackbox_no1;
        BlackBox_no1::CalculateRescueBitErrorRate(snr_db, linkBitER, constellationSize, spectralEfficiency, outputBlackbox_no1);

        return BlackBox_no2::CalculateRescueBitErrorNumber(outputBlackbox_no1.m_mi, outputBlackbox_no1.m_p_floor, snr_db.size(), constellationSize, block_length);
    }

    int
    BlackBox_no2::CalculateRescueBitErrorNumber(std::vector<double> snr_db, std::vector<double> linkBitER, std::vector<int> linkConstellationSize, std::vector<double> linkSpectralEfficiency, int block_length) {
        NS_ASSERT(snr_db.size() == linkBitER.size());
        NS_ASSERT(snr_db.size() == linkConstellationSize.size());
        NS_ASSERT(snr_db.size() == linkSpectralEfficiency.size());

        int numErrors(-1);
        std::vector<BlackBox_no2Entry> arrangeBER; // use the packets with the best ber

        if (snr_db.size() > 3) // the lookup table offers just values for three parallel links
        {
            for (uint32_t i = 0; i < snr_db.size(); ++i) {
                arrangeBER.push_back(BlackBox_no2Entry(snr_db[i], linkBitER[i], linkConstellationSize[i], linkSpectralEfficiency[i])); // fill vector arrangeBER with all links
            }

            std::sort(arrangeBER.begin(), arrangeBER.end(), sortByBER); // sort according the best ber

            snr_db.clear();
            linkBitER.clear();
            linkConstellationSize.clear();
            linkSpectralEfficiency.clear();

            for (uint32_t i = 0; i < 3; ++i) // refill snr_db, linkBitER with the best ber
            {
                snr_db.push_back(arrangeBER[i].m_snr);
                linkBitER.push_back(arrangeBER[i].m_ber);
                linkConstellationSize.push_back(arrangeBER[i].m_constellationSize);
                linkSpectralEfficiency.push_back(arrangeBER[i].m_spectralEfficiency);
            }
        }

        OutputBlackbox_no1 outputBlackbox_no1;
        BlackBox_no1::CalculateRescueBitErrorRate(snr_db, linkBitER, linkConstellationSize, linkSpectralEfficiency, outputBlackbox_no1);

        if (BlackBox_no2::TestAllEntriesEqual(linkConstellationSize))
            numErrors = BlackBox_no2::CalculateRescueBitErrorNumber(outputBlackbox_no1.m_mi, outputBlackbox_no1.m_p_floor, snr_db.size(), linkConstellationSize[0], block_length); // Blackbox_no2 all entries equal
        else
            numErrors = BlackBox_no2::CalculateRescueBitErrorNumber(outputBlackbox_no1.m_mi, outputBlackbox_no1.m_p_floor, BlackBox_no2::DetermineModulationCase(linkConstellationSize), 416, block_length);

        return numErrors;
    }

    double
    BlackBox_no2::Interpolation(std::vector<double> x, std::vector<double> y, double x_query) { // the x list needs to be sorted

        NS_ASSERT(x.size() > 1);
        NS_ASSERT(y.size() > 1);

        double x0, y0, x1, y1;

        if (x[0] < x[1]) {
            for (int i = 0; i < (int) x.size() - 1; ++i) {
                if (x_query < x[0])
                    NS_LOG_WARN("BlackBox_no2::Interpolation: X value is out of Range. x=" << x_query << " < x_edge=" << x[0]);
                else if (x[x.size() - 1] < x_query)
                    NS_LOG_WARN("BlackBox_no2::Interpolation: X value is out of Range. x=" << x_query << " > x_edge=" << x[x.size() - 1]);

                if (x_query <= x[i + 1]) {
                    x0 = x[i];
                    y0 = y[i];
                    x1 = x[i + 1];
                    y1 = y[i + 1];
                    break;
                }
            }
        } else {
            if (x_query > x[0])
                NS_LOG_WARN("BlackBox_no2::Interpolation: X value is out of Range. x=" << x_query << " > x_edge=" << x[0]);
            else if (x[x.size() - 1] > x_query)
                NS_LOG_WARN("BlackBox_no2::Interpolation: X value is out of Range. x=" << x_query << " < x_edge=" << x[x.size() - 1]);

            for (int i = 0; i < (int) x.size() - 1; ++i) {
                if (x_query >= x[i + 1]) {
                    x0 = x[i + 1];
                    y0 = y[i + 1];
                    x1 = x[i];
                    y1 = y[i];
                    break;
                }
            }
        }

        return y0 + (y1 - y0) / (x1 - x0) * (x_query - x0);
    }

    bool
    BlackBox_no2::TestAllEntriesEqual(std::vector<int> vector) {
        bool isEqual(true);

        for (uint8_t i = 0; i < vector.size(); ++i) {
            if (vector[0] != vector[i]) {
                isEqual = false;
                break;
            }
        }

        return isEqual;
    }

    int
    BlackBox_no2::DetermineModulationCase(std::vector<int> vector) {
        int qpsk(0);
        int qam16(0);
        int modulationCase(0);

        for (uint8_t i = 0; i < vector.size(); ++i) {
            if (vector[i] == 4)
                qpsk++;
            else if (vector[i] == 16)
                qam16++;
            else
                NS_FATAL_ERROR("DetermineModulationCase: Not supported modulation.");
        }

        if (qpsk == 1 && qam16 == 1)
            modulationCase = 11;
        else if (qpsk == 2 && qam16 == 1)
            modulationCase = 21;
        else if (qpsk == 1 && qam16 == 2)
            modulationCase = 12;
        else
            NS_FATAL_ERROR("DetermineModulationCase: Not supported modulation constellation.");

        return modulationCase;
    }

} // namespace ns3
