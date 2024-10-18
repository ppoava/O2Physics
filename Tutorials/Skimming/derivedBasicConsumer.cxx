// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
/// \author Nima Zardoshti <nima.zardoshti@cern.ch>, CERN

// O2 includes
#include "ReconstructionDataFormats/Track.h"
#include "Framework/AnalysisTask.h"
#include "Framework/AnalysisDataModel.h"
#include "Framework/ASoAHelpers.h"
#include "Common/DataModel/TrackSelectionTables.h"
#include "DataModel/DerivedExampleTable.h"

using namespace o2;
using namespace o2::framework;
using namespace o2::framework::expressions;

#include "Framework/runDataProcessing.h"

struct DerivedBasicConsumer {
  /// Function to aid in calculating delta-phi
  /// \param phi1 first phi value
  /// \param phi2 second phi value
  Double_t ComputeDeltaPhi(Double_t phi1, Double_t phi2)
  {
    Double_t deltaPhi = phi1 - phi2;
    if (deltaPhi < -TMath::Pi() / 2.) {
      deltaPhi += 2. * TMath::Pi();
    }
    if (deltaPhi > 3 * TMath::Pi() / 2.) {
      deltaPhi -= 2. * TMath::Pi();
    }
    return deltaPhi;
  }

  SliceCache cache;
  Partition<aod::DrTracks> associatedTracks = aod::exampleTrackSpace::pt < 6.0f && aod::exampleTrackSpace::pt > 4.0f;
  Partition<aod::DrTracks> triggerTracks = aod::exampleTrackSpace::pt > 6.0f;

  // Histogram registry: an object to hold your histograms
  HistogramRegistry histos{"histos", {}, OutputObjHandlingPolicy::AnalysisObject};
  Configurable<int> nBinsColZ{"nBinsColZ", 50, "N bins in colZ histo"};

  Filter collZFilter = nabs(aod::collision::posZ) < 10.0f;

  void init(InitContext const&)
  {
    // define axes you want to use
    const AxisSpec axisCounter{1, 0, +1, ""};
    const AxisSpec axisColZ{nBinsColZ, -20, 20, "colZ"};
    const AxisSpec axisPtAs{100, 0, 10, "ptAs"};
    const AxisSpec axisPtTr{100, 0, 10, "ptTr"};
    const AxisSpec axisDPhi{100, -0.5*TMath::Pi(), +1.5*TMath::Pi(), "#Delta#phi"};
    const AxisSpec axisDEta{100, -1.0, +1.0, "#Delta#eta"};
    histos.add("eventCounter", "eventCounter", kTH1F, {axisCounter});
    histos.add("hColZ", "hColZ", kTH1F, {axisColZ});
    histos.add("hPtAs", "hPtAs", kTH1F, {axisPtAs});
    histos.add("hPtTr", "hPtTr", kTH1F, {axisPtTr});
    histos.add("correlationFunction", "correlationFunction", kTH1F, {axisDPhi});
    histos.add("correlationFunction2d", "correlationFunction2d", kTH2F, {axisDPhi, axisDEta}); 
  }

  void process(soa::Filtered<aod::DrCollisions>::iterator const& collision, aod::DrTracks const& tracks)
  {
    histos.fill(HIST("eventCounter"), 0.5);
    histos.fill(HIST("hColZ"), collision.posZ());

    auto assoTracksThisCollision = associatedTracks->sliceByCached(aod::exampleTrackSpace::drCollisionId, collision.globalIndex(), cache);
    auto trigTracksThisCollision = triggerTracks->sliceByCached(aod::exampleTrackSpace::drCollisionId, collision.globalIndex(), cache);

    for (auto& track : assoTracksThisCollision)
      histos.fill(HIST("hPtAs"), track.pt());
    for (auto& track : trigTracksThisCollision)
      histos.fill(HIST("hPtTr"), track.pt());

    for (auto& [trigger, associated] : combinations(o2::soa::CombinationsFullIndexPolicy(trigTracksThisCollision, assoTracksThisCollision))) {
      histos.fill(HIST("correlationFunction"), ComputeDeltaPhi(trigger.phi(),associated.phi()));
      histos.fill(HIST("correlationFunction2d"), ComputeDeltaPhi(trigger.phi(),associated.phi()), trigger.eta() - associated.eta()); }
  }
};

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  WorkflowSpec workflow{adaptAnalysisTask<DerivedBasicConsumer>(cfgc, TaskName{"derived-basic-consumer"})};
  return workflow;
}
