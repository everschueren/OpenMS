// --------------------------------------------------------------------------
//                   OpenMS -- Open-Source Mass Spectrometry
// --------------------------------------------------------------------------
// Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
// ETH Zurich, and Freie Universitaet Berlin 2002-2014.
//
// This software is released under a three-clause BSD license:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of any author or any participating institution
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
// For a full list of authors, refer to the file AUTHORS.
// --------------------------------------------------------------------------
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL ANY OF THE AUTHORS OR THE CONTRIBUTING
// INSTITUTIONS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// --------------------------------------------------------------------------
// $Maintainer: Stephan Aiche $
// $Authors: Stephan Aiche, Chris Bielow $
// --------------------------------------------------------------------------

#include <OpenMS/ANALYSIS/QUANTITATION/IsobaricChannelExtractor.h>

#include <OpenMS/KERNEL/RangeUtils.h>
#include <cmath>

namespace OpenMS
{

  IsobaricChannelExtractor::IsobaricChannelExtractor(const IsobaricQuantitationMethod* const quant_method) :
    DefaultParamHandler("IsobaricChannelExtractor"),
    quant_method_(quant_method),
    selected_activation_(""),
    reporter_mass_shift_(0.1),
    min_precursor_intensity_(1.0),
    keep_unannotated_precursor_(true),
    min_reporter_intensity_(0.0),
    remove_low_intensity_quantifications_(false),
    min_precursor_purity_(0.0),
    max_precursor_isotope_deviation_(0.02)
  {
    setDefaultParams_();
  }

  IsobaricChannelExtractor::IsobaricChannelExtractor(const IsobaricChannelExtractor& other) :
    DefaultParamHandler(other),
    quant_method_(other.quant_method_),
    selected_activation_(other.selected_activation_),
    reporter_mass_shift_(other.reporter_mass_shift_),
    min_precursor_intensity_(other.min_precursor_intensity_),
    keep_unannotated_precursor_(other.keep_unannotated_precursor_),
    min_reporter_intensity_(other.min_reporter_intensity_),
    remove_low_intensity_quantifications_(other.remove_low_intensity_quantifications_),
    min_precursor_purity_(other.min_precursor_purity_),
    max_precursor_isotope_deviation_(other.max_precursor_isotope_deviation_)
  {
  }

  IsobaricChannelExtractor& IsobaricChannelExtractor::operator=(const IsobaricChannelExtractor& rhs)
  {
    if (this == &rhs)
      return *this;

    DefaultParamHandler::operator=(rhs);
    quant_method_ = rhs.quant_method_;
    selected_activation_ = rhs.selected_activation_;
    reporter_mass_shift_ = rhs.reporter_mass_shift_;
    min_precursor_intensity_ = rhs.min_precursor_intensity_;
    keep_unannotated_precursor_ = rhs.keep_unannotated_precursor_;
    min_reporter_intensity_ = rhs.min_reporter_intensity_;
    remove_low_intensity_quantifications_ = rhs.remove_low_intensity_quantifications_;
    min_precursor_purity_ = rhs.min_precursor_purity_;
    max_precursor_isotope_deviation_ = rhs.max_precursor_isotope_deviation_;

    return *this;
  }

  void IsobaricChannelExtractor::setDefaultParams_()
  {
    defaults_.setValue("select_activation", Precursor::NamesOfActivationMethod[Precursor::HCID], "Operate only on MSn scans where any of its precursors features a certain activation method (e.g., usually HCD for iTRAQ). Set to empty string if you want to disable filtering.");
    StringList activation_list;
    activation_list.insert(activation_list.begin(), Precursor::NamesOfActivationMethod, Precursor::NamesOfActivationMethod + Precursor::SIZE_OF_ACTIVATIONMETHOD - 1);
    activation_list.push_back(""); // allow disabling this

    defaults_.setValidStrings("select_activation", activation_list);

    defaults_.setValue("reporter_mass_shift", 0.1, "Allowed shift (left to right) in Da from the expected position.");
    defaults_.setMinFloat("reporter_mass_shift", 0.00000001);
    defaults_.setMaxFloat("reporter_mass_shift", 0.5);

    defaults_.setValue("min_precursor_intensity", 1.0, "Minimum intensity of the precursor to be extracted. MS/MS scans having a precursor with a lower intensity will not be considered for quantitation.");
    defaults_.setMinFloat("min_precursor_intensity", 0.0);

    defaults_.setValue("keep_unannotated_precursor", "true", "Flag if precursor with missing intensity value or missing precursor spectrum should be included or not.");
    defaults_.setValidStrings("keep_unannotated_precursor", ListUtils::create<String>("true,false"));

    defaults_.setValue("min_reporter_intensity", 0.0, "Minimum intenesity of the individual reporter ions to be used extracted.");
    defaults_.setMinFloat("min_reporter_intensity", 0.0);

    defaults_.setValue("discard_low_intensity_quantifications", "false", "Remove all reporter intensities if a single reporter is below the threshold given in min_reporter_intensity.");
    defaults_.setValidStrings("discard_low_intensity_quantifications", ListUtils::create<String>("true,false"));

    defaults_.setValue("min_precursor_purity", 0.0, "Minimum fraction of the total intensity in the isolation window of the precursor spectrum attributable to the selected precursor.");
    defaults_.setMinFloat("min_precursor_purity", 0.0);
    defaults_.setMaxFloat("min_precursor_purity", 1.0);

    defaults_.setValue("precursor_isotope_deviation", 0.02, "Maximum allowed deviation between theoretical and observed isotopic peaks of the precursor peak in the isolation window to be counted as part of the precursor.");
    defaults_.setMinFloat("precursor_isotope_deviation", 0.0);
    defaults_.addTag("precursor_isotope_deviation", "advanced");

    defaultsToParam_();
  }

  void IsobaricChannelExtractor::updateMembers_()
  {
    selected_activation_ = getParameters().getValue("select_activation");
    reporter_mass_shift_ = getParameters().getValue("reporter_mass_shift");
    min_precursor_intensity_ = getParameters().getValue("min_precursor_intensity");
    keep_unannotated_precursor_ = getParameters().getValue("keep_unannotated_precursor") == "true";
    min_reporter_intensity_ = getParameters().getValue("min_reporter_intensity");
    remove_low_intensity_quantifications_ = getParameters().getValue("discard_low_intensity_quantifications") == "true";
    min_precursor_purity_ = getParameters().getValue("min_precursor_purity");
    max_precursor_isotope_deviation_ = getParameters().getValue("precursor_isotope_deviation");
  }

  bool IsobaricChannelExtractor::isValidPrecursor_(const Precursor& precursor) const
  {
    return (!(precursor.getIntensity() > 0.0) && keep_unannotated_precursor_) || !(precursor.getIntensity() < min_precursor_intensity_);
  }

  bool IsobaricChannelExtractor::hasLowIntensityReporter_(const ConsensusFeature& cf) const
  {
    for (ConsensusFeature::const_iterator cf_it = cf.begin();
         cf_it != cf.end();
         ++cf_it)
    {
      if (cf_it->getIntensity() == 0.0)
      {
        return true;
      }
    }

    return false;
  }

  double IsobaricChannelExtractor::computePrecursorPurity_(const MSExperiment<Peak1D>::ConstIterator& ms2_spec, const PuritySate& pState) const
  {
    const double tolerance_in_ppm = 10.0;

    // we cannot analyze precursors without a charge
    if (ms2_spec->getPrecursors()[0].getCharge() == 0)
      return 1.0;

    // compute the
    const double charge_dist = Constants::NEUTRON_MASS_U / (double) ms2_spec->getPrecursors()[0].getCharge();

    // the actual boundary values
    const double strict_lower_mz = ms2_spec->getPrecursors()[0].getMZ() - ms2_spec->getPrecursors()[0].getIsolationWindowLowerOffset();
    const double strict_upper_mz = ms2_spec->getPrecursors()[0].getMZ() + ms2_spec->getPrecursors()[0].getIsolationWindowUpperOffset();

    const double fuzzy_lower_mz = strict_lower_mz - (strict_lower_mz * tolerance_in_ppm / 1000000);
    const double fuzzy_upper_mz = strict_upper_mz + (strict_upper_mz * tolerance_in_ppm / 1000000);

    // first find the actual precursor peak
    Size precursor_peak_idx = pState.precursorScan->findNearest(ms2_spec->getPrecursors()[0].getMZ());
    const Peak1D& precursor_peak = (*pState.precursorScan)[precursor_peak_idx];

    std::cerr << "Analyzing precursor window [" << strict_lower_mz << ", " << strict_upper_mz << "]" << std::endl;
    std::cerr << "Precursor peak: " << precursor_peak << " (-" << ms2_spec->getPrecursors()[0].getIsolationWindowLowerOffset() << " / +" << ms2_spec->getPrecursors()[0].getIsolationWindowUpperOffset() << ")" << std::endl;
    std::cerr << "Charge: " << ms2_spec->getPrecursors()[0].getCharge() << " charge_dist: " << charge_dist << std::endl;

    Peak1D::IntensityType precursor_intensity = precursor_peak.getIntensity();
    Peak1D::IntensityType total_intensity = precursor_peak.getIntensity();

    // go left and check for potential isotope peaks
    int idx = static_cast<int>(precursor_peak_idx) - 1;

    // last and expected next peak values
    double last_matching_mz = precursor_peak.getMZ();
    double expected_next_mz = last_matching_mz - charge_dist;


    // reasonably large value to ensure that also at the beginning, the difference will decrease
    double previous_diff = 1000.0;
    double min_diff = 1000.0;
    double min_diff_intensity = 0.0;
    double min_diff_mz = 0.0;

    std::cerr << "Expect next isotopic peak at m/z: " << expected_next_mz << std::endl;

    // check if we are still in the boundaries of our
    while (idx >= 0 && (*pState.precursorScan)[idx].getMZ() > fuzzy_lower_mz)
    {
      // we do a look ahead to know when we are in the last iteration
      bool is_last_scan = (idx == 0 || (*pState.precursorScan)[idx - 1].getMZ() <= fuzzy_lower_mz);

      // check if we take the total intensity or just a fraction
      double intensity_contribution;
      if ((*pState.precursorScan)[idx].getMZ() > strict_lower_mz)
      {
        intensity_contribution = (*pState.precursorScan)[idx].getIntensity();
      }
      else
      {
        // we're in the fuzzy area, so we will take only 50% of the given intensity
        // since we assume that the isolation window borders are not sharp
        intensity_contribution = 0.5 * (*pState.precursorScan)[idx].getIntensity();
      }

      // contribution to toal intensity
      total_intensity += intensity_contribution;

      // check if it is an isotope peak
      double current_diff = std::fabs((*pState.precursorScan)[idx].getMZ() - expected_next_mz);

      std::cerr << "Current peak: " << (*pState.precursorScan)[idx] << " (delta: " << current_diff << ")" << std::endl;

      // difference increases again -> current min seams to be a match or there is no peak at all
      // or we have the last scan so we will try to match the current state against the isotopes
      if (current_diff > previous_diff || is_last_scan)
      {
        if (std::fabs(min_diff - charge_dist) < (charge_dist * tolerance_in_ppm / 1000000))
        {
          // this is an isotopic peak, update intensity ..
          precursor_intensity += min_diff_intensity;
          // last and next pos
          last_matching_mz = min_diff_mz;
          expected_next_mz = last_matching_mz - charge_dist;

          std::cerr << "Expect next isotopic peak at m/z: " << expected_next_mz << std::endl;
        }
        else
        {
          // the peak didn't satisfy the required constraints
          // -> set values to theoretical positions
          last_matching_mz = expected_next_mz;
          expected_next_mz = last_matching_mz - charge_dist;
          std::cerr << "Expect next isotopic peak at m/z: " << expected_next_mz << std::endl;
        }

        // update currentDiff/minDiff to next pos, such that the diffs decrease again
        current_diff = std::fabs((*pState.precursorScan)[idx].getMZ() - expected_next_mz);
        min_diff = std::fabs((*pState.precursorScan)[idx].getMZ() - expected_next_mz);
      }
      else if (current_diff < min_diff)
      {
        // update the minima if diff decreased
        min_diff = current_diff;
        min_diff_intensity = intensity_contribution;
        min_diff_mz = (*pState.precursorScan)[idx].getMZ();
      }

      // next peak
      previous_diff = current_diff;
      --idx;
    }

    // now go to the right
    // go left and check for potential isotope peaks
    idx = static_cast<int>(precursor_peak_idx) + 1;

    // last and expected next peak values
    last_matching_mz = precursor_peak.getMZ();
    expected_next_mz = last_matching_mz + charge_dist;


    // reasonably large value to ensure that also at the beginning, the difference will decrease
    previous_diff = 1000.0;
    min_diff = 1000.0;
    min_diff_intensity = 0.0;
    min_diff_mz = 0.0;

    std::cerr << "Expect next isotopic peak at m/z: " << expected_next_mz << std::endl;

    // check if we are still in the boundaries of our
    while (idx < static_cast<int>((*pState.precursorScan).size()) && (*pState.precursorScan)[idx].getMZ() < fuzzy_upper_mz)
    {
      // we do a look ahead to know when we are in the last iteration
      bool is_last_scan = (idx == 0 || (*pState.precursorScan)[idx - 1].getMZ() <= fuzzy_lower_mz);

      // check if we take the total intensity or just a fraction
      double intensity_contribution;
      if ((*pState.precursorScan)[idx].getMZ() < strict_upper_mz)
      {
        intensity_contribution = (*pState.precursorScan)[idx].getIntensity();
      }
      else
      {
        // we're in the fuzzy area, so we will take only 50% of the given intensity
        // since we assume that the isolation window borders are not sharp
        intensity_contribution = 0.5 * (*pState.precursorScan)[idx].getIntensity();
      }

      // contribution to toal intensity
      total_intensity += intensity_contribution;

      // check if it is an isotope peak
      double current_diff = std::fabs((*pState.precursorScan)[idx].getMZ() - expected_next_mz);

      std::cerr << "Current peak: " << (*pState.precursorScan)[idx] << " (delta: " << current_diff << ")" << std::endl;

      // difference increases again -> current min seams to be a match or there is no peak at all
      // or we have the last scan so we will try to match the current state against the isotopes
      if (current_diff > previous_diff || is_last_scan)
      {
        if (std::fabs(min_diff - charge_dist) < (charge_dist * tolerance_in_ppm / 1000000))
        {
          // this is an isotopic peak, update intensity ..
          precursor_intensity += min_diff_intensity;
          // last and next pos
          last_matching_mz = min_diff_mz;
          expected_next_mz = last_matching_mz + charge_dist;
          std::cerr << "Expect next isotopic peak at m/z: " << expected_next_mz << std::endl;
        }
        else
        {
          // the peak didn't satisfy the required constraints
          // -> set values to theoretical positions
          last_matching_mz = expected_next_mz;
          expected_next_mz = last_matching_mz + charge_dist;
          std::cerr << "Expect next isotopic peak at m/z: " << expected_next_mz << std::endl;
        }

        // update currentDiff/minDiff to next pos, such that the diffs decrease again
        current_diff = std::fabs((*pState.precursorScan)[idx].getMZ() - expected_next_mz);
        min_diff = std::fabs((*pState.precursorScan)[idx].getMZ() - expected_next_mz);
      }
      else if (current_diff < min_diff)
      {
        // update the minima if diff decreased
        min_diff = current_diff;
        min_diff_intensity = intensity_contribution;
        min_diff_mz = (*pState.precursorScan)[idx].getMZ();
      }

      // next peak
      previous_diff = current_diff;
      ++idx;
    }

    // now get the intensity of the precursor .. we assume everything in the distance of 1/c to belong to the precursor
    // for c == charge of precursor

    double purity = precursor_intensity / total_intensity;

    if (purity < 0.0 || purity > 1.0)
    {
      LOG_ERROR << "Purity computation failed: " << purity << ", " << pState.precursorScan->getNativeID() << " (signal intensity: " << precursor_intensity << ", total intensity: " << total_intensity << ")" << std::endl;
    }

    return precursor_intensity / total_intensity;
  }

  void IsobaricChannelExtractor::extractChannels(const MSExperiment<Peak1D>& ms_exp_data, ConsensusMap& consensus_map)
  {
    if (ms_exp_data.empty())
    {
      LOG_WARN << "The given file does not contain any conventional peak data, but might"
                  " contain chromatograms. This tool currently cannot handle them, sorry.\n";
      throw Exception::MissingInformation(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Experiment has no scans!");
    }

    // clear the output map
    consensus_map.clear(false);
    consensus_map.setExperimentType("labeled_MS2");

    // create predicate for spectrum checking
    LOG_INFO << "Selecting scans with activation mode: " << (selected_activation_ == "" ? "any" : selected_activation_) << "\n";
    HasActivationMethod<MSExperiment<Peak1D>::SpectrumType> isValidActivation(ListUtils::create<String>(selected_activation_));

    // now we have picked data
    // --> assign peaks to channels
    UInt64 element_index(0);

    // remember the current precusor spectrum
    PuritySate pState(ms_exp_data);

    for (MSExperiment<Peak1D>::ConstIterator it = ms_exp_data.begin(); it != ms_exp_data.end(); ++it)
    {
      // remember the last MS1 spectra as we assume it to be the precursor spectrum
      if (it->getMSLevel() ==  1)
      {
        // remember potential precursor and continue
        pState.precursorScan = it;
        continue;
      }

      if (selected_activation_ == "" || isValidActivation(*it))
      {
        // find following ms1 scan (needed for purity computation)
        if (!pState.followUpValid(it->getRT()))
        {
          // advance iterator
          pState.advanceFollowUp(it->getRT());
        }

        // check if precursor is available
        if (it->getPrecursors().empty())
        {
          throw Exception::MissingInformation(__FILE__, __LINE__, __PRETTY_FUNCTION__, String("No precursor information given for scan native ID ") + it->getNativeID() + " with RT " + String(it->getRT()));
        }

        // check precursor constraints
        if (!isValidPrecursor_(it->getPrecursors()[0]))
        {
          LOG_DEBUG << "Skip spectrum " << it->getNativeID() << ": Precursor doesn't fulfill all constraints." << std::endl;
          continue;
        }

        // check precursor purity if we have a valid precursor ..
        double precursor_purity = -1.0;
        if (pState.precursorScan != ms_exp_data.end())
        {
          precursor_purity = computePrecursorPurity_(it, pState);
          // check if purity is high enough
          if (precursor_purity < min_precursor_purity_)
          {
            LOG_DEBUG << "Skip spectrum " << it->getNativeID() << ": Precursor purity is below the threshold. [purity = " << precursor_purity << "]" << std::endl;
            continue;
          }
        }
        else
        {
          LOG_INFO << "No precursor available for spectrum: " << it->getNativeID() << std::endl;
        }

        // store RT&MZ of parent ion as centroid of ConsensusFeature
        ConsensusFeature cf;
        cf.setUniqueId();
        cf.setRT(it->getRT());
        cf.setMZ(it->getPrecursors()[0].getMZ());

        Peak2D channel_value;
        channel_value.setRT(it->getRT());
        // for each each channel
        UInt64 map_index = 0;
        Peak2D::IntensityType overall_intensity = 0;
        for (IsobaricQuantitationMethod::IsobaricChannelList::const_iterator cl_it = quant_method_->getChannelInformation().begin();
             cl_it != quant_method_->getChannelInformation().end();
             ++cl_it)
        {
          // set mz-position of channel
          channel_value.setMZ(cl_it->center);
          // reset intensity
          channel_value.setIntensity(0);

          // as every evaluation requires time, we cache the MZEnd iterator
          const MSExperiment<Peak1D>::SpectrumType::ConstIterator mz_end = it->MZEnd(cl_it->center + reporter_mass_shift_);

          // add up all signals
          for (MSExperiment<Peak1D>::SpectrumType::ConstIterator mz_it = it->MZBegin(cl_it->center - reporter_mass_shift_);
               mz_it != mz_end;
               ++mz_it)
          {
            channel_value.setIntensity(channel_value.getIntensity() + mz_it->getIntensity());
          }

          // discard contribution of this channel as it is below the required intensity threshold
          if (channel_value.getIntensity() < min_reporter_intensity_)
          {
            channel_value.setIntensity(0);
          }

          overall_intensity += channel_value.getIntensity();
          // add channel to ConsensusFeature
          cf.insert(map_index++, channel_value, element_index);
        } // ! channel_iterator

        // check if we keep this feature or if it contains low-intensity quantifications
        if (remove_low_intensity_quantifications_ && hasLowIntensityReporter_(cf))
        {
          continue;
        }

        // check featureHandles are not empty
        if (!(overall_intensity > 0))
        {
          cf.setMetaValue("all_empty", String("true"));
        }
        // add purity information if we could compute it
        if (precursor_purity > 0.0)
        {
          cf.setMetaValue("precursor_purity", precursor_purity);
        }

        // embed the id of the scan from which the quantitative information was extracted
        cf.setMetaValue("scan_id", it->getNativeID());
        // .. as well as additional meta information
        cf.setMetaValue("precursor_intensity", it->getPrecursors()[0].getIntensity());
        cf.setMetaValue("precursor_charge", it->getPrecursors()[0].getCharge());

        cf.setIntensity(overall_intensity);
        consensus_map.push_back(cf);

        // the tandem-scan in the order they appear in the experiment
        ++element_index;
      }
    } // ! Experiment iterator

    /// add meta information to the map
    registerChannelsInOutputMap_(consensus_map);
  }

  void IsobaricChannelExtractor::registerChannelsInOutputMap_(ConsensusMap& consensus_map)
  {
    // register the individual channels in the output consensus map
    Int index = 0;
    for (IsobaricQuantitationMethod::IsobaricChannelList::const_iterator cl_it = quant_method_->getChannelInformation().begin();
         cl_it != quant_method_->getChannelInformation().end();
         ++cl_it)
    {
      ConsensusMap::FileDescription channel_as_map;
      // label is the channel + description provided in the Params
      channel_as_map.label = quant_method_->getName() + "_" + cl_it->name;

      // TODO(aiche): number of features need to be set later
      channel_as_map.size = consensus_map.size();

      // add some more MetaInfo
      channel_as_map.setMetaValue("channel_name", cl_it->name);
      channel_as_map.setMetaValue("channel_id", cl_it->id);
      channel_as_map.setMetaValue("channel_description", cl_it->description);
      channel_as_map.setMetaValue("channel_center", cl_it->center);
      consensus_map.getFileDescriptions()[index++] = channel_as_map;
    }
  }

} // namespace
