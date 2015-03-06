/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
// Implemented from Sebastian Kühlmorgen TU Dresden, derived from Matlab code from Petri Komulainen UOULU
/*
This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; 
either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program; 
if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef BLACKBOX_H
#define BLACKBOX_H

#include "ns3/log.h"

#include "blackbox.h"

#include <math.h>
#include <numeric>

NS_LOG_COMPONENT_DEFINE ("BlackBox");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED (BlackBox);

TypeId
BlackBox::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BlackBox")
    .SetParent<Object> ()
    .AddConstructor<BlackBox> ()
  ;
  return tid;
}

BlackBox::BlackBox ()
{
}

BlackBox::~BlackBox ()
{
}

double
BlackBox::CalculateRescuePER(std::vector<double> snr_db, std::vector<double> linkPER, double constellationSize, double spectralEfficiency, int packetLength)
{
  NS_LOG_FUNCTION ("");
  for (std::vector<double>::iterator it = snr_db.begin(); it != snr_db.end(); ++it)
    {
      NS_LOG_DEBUG ("snr = " << *it);
    }
  for (std::vector<double>::iterator it = linkPER.begin(); it != linkPER.end(); ++it)
    {
      NS_LOG_DEBUG ("per = " << *it);
    }


  std::vector<double> p_tx = BlackBox::PERtoBER(linkPER, packetLength); // convert PER in BER (packet length in bit)

  NS_ASSERT(snr_db.size() == linkPER.size());
  int num_links = snr_db.size(); // number of incoming links

  std::vector<double> esN0;
  for (std::vector<double>::iterator it = snr_db.begin(); it != snr_db.end(); ++it)
    {
      esN0.push_back(std::pow(10, *it / 10)); // convert SNR to linear scale
    }

  for (std::vector<double>::iterator it = esN0.begin(); it != esN0.end(); ++it)
    {
      NS_LOG_DEBUG ("esN0 = " << *it);
    }

  std::vector<double> C;
  for (std::vector<double>::iterator it = esN0.begin(); it != esN0.end(); ++it)
    {
      // Determine constellation constrained capacity per complex symbol
      if (constellationSize == 2)       // BPSK
        C.push_back(BlackBox::J_Function(std::sqrt(8 * *it)));
      else if (constellationSize == 4)  // QPSK
        C.push_back(2 * BlackBox::J_Function(std::sqrt(4 * *it)));
      else if (constellationSize == 16) // 16QAM
        C = Four_Pam_Capacity_Table_Lookup(snr_db); // the multiplication by 2 take place in the function
      else
        {
          NS_LOG_DEBUG ("Unsupported constellation size.");
          return 0;
        }
    }

  // Phi = constellation constrained mutual information per information bit i.e. channel capacity normalized with spectral efficiency
  // Note that this can be >1
  std::vector<double> Phi;
  for (std::vector<double>::iterator it = C.begin(); it != C.end(); ++it)
    {
      Phi.push_back(*it / spectralEfficiency);
    }

  for (std::vector<double>::iterator it = Phi.begin(); it != Phi.end(); ++it)
    {
      NS_LOG_DEBUG ("Phi = " << *it);
    }

  double H; // binary entropy function
  std::vector<double> MI;

  // Calculate mutual information per transmitter, and the sum
  for (int i = 0; i < num_links; i++)
    {
      if (p_tx[i] > 0)
        {
          H = -(1 - p_tx[i]) * log2(1 - p_tx[i]) - p_tx[i] * log2(p_tx[i]);
          MI.push_back(std::min((1 - H) * Phi[i], 1 - H)); // MI increment always < 1
        }
      else
        MI.push_back(Phi[i]); // if no errors in transmitter, allow MI increment > 1
    }

  for (std::vector<double>::iterator it = MI.begin(); it != MI.end(); ++it)
    {
      NS_LOG_DEBUG ("MI = " << *it);
    }
  double MI_tot = std::accumulate(MI.begin(), MI.end(), 0.0);
  NS_LOG_DEBUG ("MI_tot = " << MI_tot);

  // Determine expected bit error probability after joint decoding at the receiver
  double p_rateDistortion = BlackBox::InvH(1 - std::min(MI_tot, 1.0));
  double p_floor = BlackBox::ErrorFloor(p_tx);
  double p_rx = std::max(p_rateDistortion, p_floor); // bit error rate

  NS_LOG_DEBUG ("p_rateDistortion = " << p_rateDistortion << 
                ", p_floor = " << p_floor << 
                ", p_rx = " << p_rx);

  return BlackBox::BERtoPER(p_rx, packetLength);
}

double
BlackBox::J_Function(double sigma)
{
  double x = 0;
  double y = 0;

  const double H1 = 0.3073;
  const double H2 = 0.8935;
  const double H3 = 1.1064;

  if (sigma >= 0)
    {
      x = -H1 * std::pow(sigma, 2 * H2);
      y = 1 - std::pow(2, x);
    }

  NS_LOG_DEBUG ("J-function: sigma: " << sigma << 
                ", x: " << x << 
                ", y: " << y << 
                ", return: " << std::pow(y, H3));
  return std::pow(y, H3);
}

std::vector<double>
BlackBox::Four_Pam_Capacity_Table_Lookup(std::vector<double> snr_DB)
{
  int num_links = snr_DB.size();
  std::vector<double> C;
  double snr_lower_dB;
  int lower_ind;
  double d;

  for (int i = 0; i < num_links; ++i)
    {
      if (snr_DB[i] < -35.0)    // -infinity < snr < -35
        C.push_back(0.0);
      else if (snr_DB[i] >= 25) // 25 <= snr < infinity
        C.push_back(2.0);
      else
        {
          snr_lower_dB = 0.5 * std::floor(2 * snr_DB[i]); // with 0.5 dB granularity
          lower_ind = round(2 * (snr_lower_dB + 35) + 1); // lower index in table
          d = (snr_DB[i] - snr_lower_dB) / 0.5;           // normalized distance from lower limit (always non-negative)
          C.push_back( 2*( (1 - d)*capacity_table[lower_ind - 1] + d*capacity_table[lower_ind]) );
        }
    }

  return C;
}

double
BlackBox::InvH(double x)
{
  double p;

  if (x <= 0 || std::abs(x - 0) <= 1e-6)
    p = 0;
  else if (x > 1)
    p = 0.5;
  else // 0 < x <= 1
    {
      p = 0.5;              // start from the end of search area 0.0 ... 0.5
      double delta = x - 1; // x - H(0.5)
      double step = 0.25;   // distance to the next middle point
      while (std::abs(delta) > 1e-6)
        {
          p = p + ((delta > 0) - (delta < 0)) * step;      // sign function = (delta > 0) - (delta < 0)
          delta = x + p * log2(p) + (1 - p) * log2(1 - p); // x-H(p)
          step = 0.5 * step;
        }
    }

  return p;
}

double
BlackBox::ErrorFloor(std::vector<double> p_tx)
{
  int numLinks = p_tx.size();
  bool zeroErrorLink = false;
  double p_floor = 0;
  std::vector<double> bitDivisor;
  std::vector<double> Ls;
  // if there is at least one zero link error probability, error floor disappears
  for (std::vector<double>::iterator it = p_tx.begin(); it != p_tx.end(); ++it)
    {
      if (*it == 0)
        {
          zeroErrorLink = true;
          break;
        }
    }

  if (zeroErrorLink)
    p_floor = 0;
  else
    {
      bitDivisor = BlackBox::GenerateBitDivisor(numLinks); // (1, 2, 4, 8, 16, ...)

      for (std::vector<double>::iterator it = p_tx.begin(); it != p_tx.end(); ++it)
        {
          Ls.push_back(std::log((1 - *it) / *it)); // non-negative numbers, LLRs per link after weighting by f_c-function
        }

      // go through all error event combinations (combination==0 means that all links are correct)
      p_floor = 0;

      for (int combination = 0; combination < std::pow(2.0, numLinks) - 1; ++combination) // each bit represents state of one link
        {
          // calculate the probability of the combination and the corresponding soft decision
          double p_combination = 1;
          double L = 0;

          for (int i = 0; i < numLinks; ++i)
            {
              // find the bit (0 or 1) corresponding to link 'i' (just bit shift to right and mask the right-most bit)
              int bit = (int) std::floor((double) combination / bitDivisor[i]) % 2;

              if (bit == 1) // no error
                {
                  p_combination = p_combination * (1 - p_tx[i]);
                  L = L + Ls[i];
                }
              else // error in link
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
BlackBox::PERtoBER(std::vector<double> linkPER, int packetLength)
{
  std::vector<double> BER;
  for (std::vector<double>::iterator it = linkPER.begin(); it != linkPER.end(); ++it)
    {
      BER.push_back(1 - std::pow(1.0 - *it, 1.0 / (double) packetLength)); // convert PER to BER Pbit = 1 - (1 - Ppack)^(1/N)  N...packet length
    }

  return BER;
}

double
BlackBox::BERtoPER(double ber, int packetLength)
{
  return (1 - std::pow(1.0 - ber, packetLength)); // convert BER to PER Ppack = 1 - (1 - Pbit)^(N)  N...packet length
}

std::vector<double>
BlackBox::GenerateBitDivisor(int numberLinks)
{
  std::vector<double> bitDivisor;

  for (int i = 0; i < numberLinks; i++)
    {
      bitDivisor.push_back(std::pow(2.0, (double) i));
    }

  return bitDivisor;
}

} // namespace ns3

#endif /* BLACKBOX_H */
