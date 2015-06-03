/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "blackbox_no2.h"

NS_LOG_COMPONENT_DEFINE ("BlackBox_no2");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED (BlackBox_no2);

TypeId
BlackBox_no2::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BlackBox_no2")
    .SetParent<Object> ()
    .AddConstructor<BlackBox_no2> ()
  ;
  return tid;
}

BlackBox_no2::BlackBox_no2 ()
{
}

BlackBox_no2::~BlackBox_no2 ()
{
}

int
BlackBox_no2::CalculateRescueBitErrorNumber (double MI, double p_floor, int n_link, int constellationSize)
{
  NS_ASSERT (n_link < 4);

  NS_LOG_FUNCTION ("MI: " << MI << "p_floor: " << p_floor << "n_link:" << n_link << "constellationSize: " << constellationSize);
  std::vector<double> mi_tab;
  std::vector<double> alpha_tab;
  std::vector<double> mu_tab;
  std::vector<double> sigma_tab;
  std::vector<double> ber_tab;
  std::vector<double> ber_tabLog;

  switch (constellationSize)
    {
    case 2:
      switch (n_link)
        {
        case 1:
          mi_tab     = Fill (BPSK_1_mi, sizeof (BPSK_1_mi) / sizeof (double));
          alpha_tab  = Fill (BPSK_1_alpha, sizeof (BPSK_1_alpha) / sizeof (double));
          mu_tab     = Fill (BPSK_1_mu, sizeof (BPSK_1_mu) / sizeof (double));
          sigma_tab  = Fill (BPSK_1_sigma, sizeof (BPSK_1_sigma) / sizeof (double));
          ber_tab    = Fill (BPSK_1_ber, sizeof (BPSK_1_ber) / sizeof (double));
          ber_tabLog = Fill (BPSK_1_ber_Log, sizeof (BPSK_1_ber_Log) / sizeof (double));
          break;
        case 2:
          mi_tab     = Fill (BPSK_2_mi, sizeof (BPSK_2_mi) / sizeof (double));
          alpha_tab  = Fill (BPSK_2_alpha, sizeof (BPSK_2_alpha) / sizeof (double));
          mu_tab     = Fill (BPSK_2_mu, sizeof (BPSK_2_mu) / sizeof (double));
          sigma_tab  = Fill (BPSK_2_sigma, sizeof (BPSK_2_sigma) / sizeof (double));
          ber_tab    = Fill (BPSK_2_ber, sizeof (BPSK_2_ber) / sizeof (double));
          ber_tabLog = Fill (BPSK_2_ber_Log, sizeof (BPSK_2_ber_Log) / sizeof (double));
          break;
        case 3:
          mi_tab     = Fill (BPSK_3_mi, sizeof (BPSK_3_mi) / sizeof (double));
          alpha_tab  = Fill (BPSK_3_alpha, sizeof (BPSK_3_alpha) / sizeof (double));
          mu_tab     = Fill (BPSK_3_mu, sizeof (BPSK_3_mu) / sizeof (double));
          sigma_tab  = Fill (BPSK_3_sigma, sizeof (BPSK_3_sigma) / sizeof (double));
          ber_tab    = Fill (BPSK_3_ber, sizeof (BPSK_3_ber) / sizeof (double));
          ber_tabLog = Fill (BPSK_3_ber_Log, sizeof (BPSK_3_ber_Log) / sizeof (double));
          break;
        default:
          NS_LOG_UNCOND ("Error: Number of parallel links at maximum 3.");
          return -1;
        }
      ;
      break;
    case 4:
      switch (n_link)
        {
        case 1:
          mi_tab     = Fill (QPSK_1_mi, sizeof (QPSK_1_mi) / sizeof (double));
          alpha_tab  = Fill (QPSK_1_alpha, sizeof (QPSK_1_alpha) / sizeof (double));
          mu_tab     = Fill (QPSK_1_mu, sizeof (QPSK_1_mu) / sizeof (double));
          sigma_tab  = Fill (QPSK_1_sigma, sizeof (QPSK_1_sigma) / sizeof (double));
          ber_tab    = Fill (QPSK_1_ber, sizeof (QPSK_1_ber) / sizeof (double));
          ber_tabLog = Fill (QPSK_1_ber_Log, sizeof (QPSK_1_ber_Log) / sizeof (double));
          break;
        case 2:
          mi_tab     = Fill (QPSK_2_mi, sizeof (QPSK_2_mi) / sizeof (double));
          alpha_tab  = Fill (QPSK_2_alpha, sizeof (QPSK_2_alpha) / sizeof (double));
          mu_tab     = Fill (QPSK_2_mu, sizeof (QPSK_2_mu) / sizeof (double));
          sigma_tab  = Fill (QPSK_2_sigma, sizeof (QPSK_2_sigma) / sizeof (double));
          ber_tab    = Fill (QPSK_2_ber, sizeof (QPSK_2_ber) / sizeof (double));
          ber_tabLog = Fill (QPSK_2_ber_Log, sizeof (QPSK_2_ber_Log) / sizeof (double));
          break;
        case 3:
          mi_tab     = Fill (QPSK_3_mi, sizeof (QPSK_3_mi) / sizeof (double));
          alpha_tab  = Fill (QPSK_3_alpha, sizeof (QPSK_3_alpha) / sizeof (double));
          mu_tab     = Fill (QPSK_3_mu, sizeof (QPSK_3_mu) / sizeof (double));
          sigma_tab  = Fill (QPSK_3_sigma, sizeof (QPSK_3_sigma) / sizeof (double));
          ber_tab    = Fill (QPSK_3_ber, sizeof (QPSK_3_ber) / sizeof (double));
          ber_tabLog = Fill (QPSK_3_ber_Log, sizeof (QPSK_3_ber_Log) / sizeof (double));
          break;
        default:
          NS_LOG_UNCOND ("Error: Number of parallel links at maximum 3.");
          return -1;
        }
      ;
      break;
    default:
      NS_LOG_UNCOND ("Error: Constellation size is not supported in this version.\n Please choose the constellation size: 2 - BPSK, 4 - QPSK\n");
      return -1;
    }

  double MI_t;

  // Determine the MI threshold
  if (p_floor >= ber_tab[0])
    MI_t = mi_tab[0];
  else if (p_floor <= ber_tab[ber_tab.size () - 1])
    MI_t = mi_tab[mi_tab.size () - 1];
  else
    MI_t = Interpolation (ber_tabLog, mi_tab, std::log (p_floor)); // find the MI threshold by linear interpolation, the theoretical error floor is x end

  if (MI > MI_t)
    MI = MI_t;

  // Get the three parameters (alpha, mu, sigma) of the BE distribution by linear interpolation
  double alpha = Interpolation (mi_tab, alpha_tab, MI);
  double mu = Interpolation (mi_tab, mu_tab, MI);
  double sigma = Interpolation (mi_tab, sigma_tab, MI);

  // Generate error count according to the BE distribution
  Ptr<UniformRandomVariable> random_unif = CreateObject<UniformRandomVariable> ();
  Ptr<NormalRandomVariable> random_norm = CreateObject<NormalRandomVariable> ();
  random_norm->SetAttribute ("Mean", DoubleValue (mu));
  random_norm->SetAttribute ("Variance", DoubleValue (sigma * sigma));

  int n_errors = -1;

  if (random_unif->GetValue () > alpha)
    {
      n_errors = round (random_norm->GetValue ());

      if (n_errors < 0)
        n_errors = 0;
    }
  else
    n_errors = 0;

  NS_LOG_FUNCTION ("Errors: " << n_errors);

  return n_errors;
}

int
BlackBox_no2::CalculateRescueBitErrorNumber (std::vector<double> snr_db, std::vector<double> linkBitER, int constellationSize, double spectralEfficiency)
{
  NS_ASSERT (snr_db.size () == linkBitER.size ());

  if (snr_db.size () >= 3) // the lookup table has just values for three parallel links
    {
      snr_db.erase (snr_db.begin () + 3, snr_db.end ());
      linkBitER.erase (linkBitER.begin () + 3, linkBitER.end ());
    }

  OutputBlackBox_no1 outputBlackBox_no1;
  BlackBox_no1::CalculateRescueBitErrorRate (snr_db, linkBitER, constellationSize, spectralEfficiency, outputBlackBox_no1);

  if (isnan (outputBlackBox_no1.mi))
    {
      NS_LOG_FUNCTION ("Errors: 4000");
      return 4000;
    }
  else
    return BlackBox_no2::CalculateRescueBitErrorNumber (outputBlackBox_no1.mi, outputBlackBox_no1.p_floor, snr_db.size (), constellationSize);   
}

int
BlackBox_no2::CalculateRescueBitErrorNumber (std::vector<double> snr_db, std::vector<double> linkBitER, std::vector<int> linkConstellationSize, std::vector<double> linkSpectralEfficiency)
{
  NS_ASSERT (snr_db.size() == linkBitER.size ());

  if (snr_db.size () >= 3) // the lookup table offers just values for three parallel links
    {
      snr_db.erase (snr_db.begin () + 3, snr_db.end ());
      linkBitER.erase (linkBitER.begin () + 3, linkBitER.end ());
      linkConstellationSize.erase (linkConstellationSize.begin () + 3, linkConstellationSize.end ());
      linkSpectralEfficiency.erase (linkSpectralEfficiency.begin () + 3, linkSpectralEfficiency.end ());
    }

  OutputBlackBox_no1 outputBlackBox_no1;
  BlackBox_no1::CalculateRescueBitErrorRate (snr_db, linkBitER, linkConstellationSize, linkSpectralEfficiency, outputBlackBox_no1);

  return BlackBox_no2::CalculateRescueBitErrorNumber (outputBlackBox_no1.mi, outputBlackBox_no1.p_floor, snr_db.size (), linkConstellationSize[0]);
}

double
BlackBox_no2::Interpolation (std::vector<double> x, std::vector<double> y, double x_query)
{ // the x list needs to be sorted

  NS_ASSERT (x.size ());
  NS_ASSERT (y.size ());

  double x0, y0, x1, y1;

  if (x[0] < x[1])
    {
      for (int i = 0; i < (int) x.size () - 1; ++i)
        {
          if(x_query < x[0])
            NS_LOG_WARN ("BlackBox_no2::Interpolation: X value is out of Range. x=" << x_query << " < x_edge=" << x[0]);
          else if (x[x.size ()-1] < x_query)
            NS_LOG_WARN ("BlackBox_no2::Interpolation: X value is out of Range. x=" << x_query << " > x_edge=" << x[x.size ()-1]);

          if (x_query <= x[i + 1])
            {
              x0 = x[i];
              y0 = y[i];
              x1 = x[i + 1];
              y1 = y[i + 1];
              break;
            }
        }
    }
  else
    {
      if (x_query > x[0])
        NS_LOG_WARN ("BlackBox_no2::Interpolation: X value is out of Range. x=" << x_query << " > x_edge=" << x[0]);
      else if (x[x.size ()-1] > x_query)
        NS_LOG_WARN ("BlackBox_no2::Interpolation: X value is out of Range. x=" << x_query << " < x_edge=" << x[x.size ()-1]);

      for (int i = 0; i < (int) x.size () - 1; ++i)
        {
          if (x_query >= x[i + 1])
            {
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

std::vector<double>
BlackBox_no2::Fill (const double arr[], int length)
{
  std::vector<double> vector;

  for (int i = 0; i < length; ++i)
    {
      vector.push_back (arr[i]);
    }

  return vector;
}

} // namespace ns3
