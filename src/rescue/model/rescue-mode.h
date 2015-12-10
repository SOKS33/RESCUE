/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 AGH Univeristy of Science and Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Lukasz Prasnal <prasnal@kt.agh.edu.pl>
 *
 * Adapted for RESCUE basing on NS-3 Wifi module by Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef RESCUE_MODE_H
#define RESCUE_MODE_H

#include <stdint.h>
#include <string>
#include <vector>
#include <ostream>
#include "ns3/attribute-helper.h"

namespace ns3 {

    /**
     * This enumeration defines the modulation classes - for possible further extension/modification
     */
    enum RescueModulationClass {
        /** Modulation class unknown or unspecified. A RescueMode with this
        RescueModulationClass has not been properly initialised. */
        RESCUE_MOD_CLASS_UNKNOWN = 0,
        /** OFDM modulation class - will there be other classes? */
        RESCUE_MOD_CLASS_OFDM
    };

    /**
     * This enumeration defines the various convolutional coding rates
     * used for the OFDM transmission modes in the RESCUE transceivers.
     * rates which do not have an explicit
     * coding stage in their generation should have this parameter set to
     * RESCUE_CODE_RATE_UNDEFINED.
     */
    enum RescueCodeRate {
        /** No explicit coding */
        RESCUE_CODE_RATE_UNDEFINED,
        /** Rate 3/4 */
        RESCUE_CODE_RATE_3_4,
        /** Rate 2/3 */
        RESCUE_CODE_RATE_2_3,
        /** Rate 1/2 */
        RESCUE_CODE_RATE_1_2,
        /** Rate 1/2 */
        RESCUE_CODE_RATE_1_4,
        /** Rate 5/6 */
        RESCUE_CODE_RATE_5_6

    };

    /**
     * \brief represent a single transmission mode
     * \ingroup rescue
     *
     * A RescueMode is implemented by a single integer which is used
     * to lookup in a global array the characteristics of the
     * associated transmission mode. It is thus extremely cheap to
     * keep a RescueMode variable around.
     */
    class RescueMode {
    public:
        /**
         * \returns the number of Hz used by this signal
         */
        uint32_t GetBandwidth(void) const;
        /**
         * \returns the physical bit rate of this signal.
         *
         * If a transmission mode uses 1/2 FEC, and if its
         * data rate is 3Mbs, the phy rate is 6Mbs
         */
        uint64_t GetPhyRate(void) const;
        /**
         * \returns the data bit rate of this signal.
         */
        uint64_t GetDataRate(void) const;
        /**
         * \returns the coding rate of this transmission mode
         */
        enum RescueCodeRate GetCodeRate(void) const;
        /**
         * \returns the size of the modulation constellation.
         */
        uint8_t GetConstellationSize(void) const;
        /**
         * \returns a human-readable representation of this RescueMode
         * instance.
         */
        std::string GetUniqueName(void) const;
        /**
         * \returns the uid associated to this wireless mode.
         *
         * Each specific wireless mode should have a different uid.
         */
        uint32_t GetUid(void) const;
        /**
         *
         * \returns the Modulation Class to which this RescueMode belongs (Currently only OFDM).
         */
        enum RescueModulationClass GetModulationClass() const;
        /**
         * \returns spectral efficiency - number of information bits per complex symbol
         * (calculated as code rate * log2 (constellation size) )
         */
        double GetSpectralEfficiency() const;

        /**
         * Create an invalid RescueMode. Calling any method on the
         * instance created will trigger an assert. This is useful
         * to separate the declaration of a RescueMode variable from
         * its initialization.
         */
        RescueMode();
        /**
         * Create a RescueMode if the given string represents a valid
         * RescueMode name.
         *
         * \param name std::string of a valid RescueMode name
         */
        RescueMode(std::string name);
    private:
        friend class RescueModeFactory;
        /**
         * Create a RescueMode from a given unique ID.
         *
         * \param uid unique ID
         */
        RescueMode(uint32_t uid);
        uint32_t m_uid;
    };

    bool operator==(const RescueMode &a, const RescueMode &b);
    std::ostream & operator<<(std::ostream & os, const RescueMode &mode);
    std::istream & operator>>(std::istream &is, RescueMode &mode);

    /**
     * \class ns3::RescueModeValue
     * \brief hold objects of type ns3::RescueMode
     */

    ATTRIBUTE_HELPER_HEADER(RescueMode);

    /**
     * In various parts of the code, folk are interested in maintaining a
     * list of transmission modes. The vector class provides a good basis
     * for this, but we here add some syntactic sugar by defining a
     * RescueModeList type, and a corresponding iterator.
     */
    typedef std::vector<RescueMode> RescueModeList;
    /**
     * An iterator for RescueModeList vector.
     */
    typedef RescueModeList::const_iterator RescueModeListIterator;

    /**
     * A list of RESCUE Modulation and Coding Scheme (MCS).
     */
    typedef std::vector<uint8_t> RescueMcsList;
    /**
     * An iterator for RescueMcsList vector.
     */
    typedef RescueMcsList::const_iterator RescueMcsListIterator;

    /**
     * \brief create RescueMode class instances and keep track of them.
     *
     * This factory ensures that each RescueMode created has a unique name
     * and assigns to each of them a unique integer.
     */
    class RescueModeFactory {
    public:
        /**
         * Create a RescueMode.
         *
         * \param uniqueName the name of the associated RescueMode. This name
         *        must be unique accross _all_ instances.
         * \param modClass the class of modulation
         * \param bandwidth the bandwidth (Hz) of the signal generated when the
         *        associated RescueMode is used.
         * \param dataRate the rate (bits/second) at which the user data is transmitted
         * \param codingRate if convolutional coding is used for this rate
         *        then this parameter specifies the convolutional coding rate
         *        used. If there is no explicit convolutional coding step then
         *        the caller should set this parameter to RESCUE_CODE_RATE_UNCODED.
         * \param constellationSize the order of the constellation used.
         * \return RescueMode
         */
        static RescueMode CreateRescueMode(std::string uniqueName,
                enum RescueModulationClass modClass,
                uint32_t bandwidth,
                uint32_t dataRate,
                enum RescueCodeRate codingRate,
                uint8_t constellationSize);

    private:
        friend class RescueMode;
        friend std::istream & operator>>(std::istream &is, RescueMode &mode);

        /**
         * Return a RescueModeFactory
         *
         * \return a RescueModeFactory
         */
        static RescueModeFactory* GetFactory();
        RescueModeFactory();

        /**
         * This is the data associated to a unique RescueMode.
         * The integer stored in a RescueMode is in fact an index
         * in an array of RescueModeItem objects.
         */
        struct RescueModeItem {
            std::string uniqueUid;
            uint32_t bandwidth;
            uint32_t dataRate;
            uint32_t phyRate;
            enum RescueModulationClass modClass;
            uint8_t constellationSize;
            enum RescueCodeRate codingRate;
        };

        /**
         * Search and return RescueMode from a given name.
         *
         * \param name human-readable RescueMode
         * \return RescueMode
         */
        RescueMode Search(std::string name);
        /**
         * Allocate a RescueModeItem from a given uniqueUid.
         *
         * \param uniqueUid
         * \return uid
         */
        uint32_t AllocateUid(std::string uniqueUid);
        /**
         * Return a RescueModeItem at the given uid index.
         *
         * \param uid
         * \return RescueModeItem at the given uid
         */
        RescueModeItem* Get(uint32_t uid);

        /**
         * typedef for a vector of RescueModeItem.
         */
        typedef std::vector<struct RescueModeItem> RescueModeItemList;
        RescueModeItemList m_itemList;
    };

} // namespace ns3

#endif /* RESCUE_MODE_H */
