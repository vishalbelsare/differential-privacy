// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef DIFFERENTIAL_PRIVACY_ACCOUNTING_PRIVACY_LOSS_DISTRIBUTION_H_
#define DIFFERENTIAL_PRIVACY_ACCOUNTING_PRIVACY_LOSS_DISTRIBUTION_H_
// Implementing Privacy loss distribution (PLD).
// The main feature of PLD is that it allows for accurate computation of
// privacy parameters under composition. Please refer to the supplementary
// material below for more details:
// ../../common_docs Privacy_Loss_Distributions.pdf

#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "absl/types/optional.h"
#include "accounting/common/common.h"
#include "accounting/privacy_loss_mechanism.h"
#include "proto/accounting/privacy-loss-distribution.pb.h"

namespace differential_privacy {
namespace accounting {
// Privacy loss distribution (PLD) of two discrete distributions,
// the upper distribution mu_upper and the lower distribution mu_lower, is
// defined as a distribution on real numbers generated by first picking o
// according to mu_upper and then outputting the privacy loss
// ln(mu_upper(o) / mu_lower(o)) where mu_lower(o) and mu_upper(o) are the
// probability masses of o in mu_lower and mu_upper respectively. This class
// allows one to create and manipulate privacy loss distributions.
//
// PLD allows one to (approximately) compute the epsilon-hockey stick
// divergence between mu_upper and mu_lower, which is defined as
// sum_{o} [mu_upper(o) - e^{epsilon} * mu_lower(o)]_+. This quantity in turn
// governs the parameter delta of (epsilon, delta)-differential privacy of the
// corresponding protocol. (See Observation 1 in the supplementary material.)
//
// The above definitions extend to continuous distributions. The PLD of two
// continuous distributions mu_upper and mu_lower is defined as a distribution
// on real numbers generated by first sampling an outcome o according to
// mu_upper and then outputting the privacy loss
// ln(f_{mu_upper}(o) / f_{mu_lower}(o)) where f_{mu_lower}(o) and
// f_{mu_upper}(o) are the probability density functions at o in mu_lower and
// mu_upper respectively. Moreover, for continuous distributions the
// epsilon-hockey stick divergence is defined as
// int [f_{mu_upper}(o) - e^{epsilon} * f_{mu_lower}(o)]_+ do.

class PrivacyLossDistribution {
 public:
  // Creates {@link PrivacyLossDistribution} from two probability mass functions
  // and some additional parameters:
  //   estimate_type: kPessimistic denoting whether the rounding is done in
  //     such a way that the resulting epsilon-hockey stick divergence
  //     computation gives an upper estimate to the real value.
  //   discretization_interval: the discretization interval for the privacy
  //     loss distribution. The values will be rounded up/down to be integer
  //     multiples of this number.
  //   mass_truncation_bound: when the log of the probability mass of the upper
  //     distribution is below this bound, it is either (i) included in
  //     infinity_mass in the case of pessimistic estimate or (ii) discarded
  //     completely in the case of optimistic estimate. The larger
  //     mass_truncation_bound is, the more error it may introduce in divergence
  //     calculations.
  static std::unique_ptr<PrivacyLossDistribution> Create(
      const ProbabilityMassFunction& pmf_lower,
      const ProbabilityMassFunction& pmf_upper,
      EstimateType estimate_type = EstimateType::kPessimistic,
      double discretization_interval = 1e-4,
      double mass_truncation_bound = -50);

  // Creates {@link PrivacyLossDistribution} corresponding to an algorithm that
  // does not leak privacy at all (i.e. output is independent of input).
  static std::unique_ptr<PrivacyLossDistribution> CreateIdentity(
      double discretization_interval = 1e-4);

  // Creates {@link PrivacyLossDistribution} from
  // {@link AdditiveNoisePrivacyLoss} and some additional parameters:
  //   estimate_type: kPessimistic denoting that the rounding is done in
  //     such a way that the resulting epsilon-hockey stick divergence
  //     computation gives an upper estimate to the real value.
  //   discretization_interval: the discretization interval for the privacy
  //     loss distribution. The values will be rounded up/down to be integer
  //     multiples of this number.
  static std::unique_ptr<PrivacyLossDistribution> CreateForAdditiveNoise(
      const AdditiveNoisePrivacyLoss& mechanism_privacy_loss,
      EstimateType estimate_type = EstimateType::kPessimistic,
      double discretization_interval = 1e-4);

  // Creates {@link PrivacyLossDistribution} for the Randomized Response with a
  // given number of buckets and a noise parameter.
  //
  // The Randomized Response over k buckets with noise parameter p takes in an
  // input which is one of the k buckets. With probability 1 - p, it simply
  // outputs the input bucket. Otherwise, with probability p, it outputs a
  // bucket drawn uniformly at random from the k buckets.
  //
  // noise_parameter: the probability that the Randomized Response outputs a
  //   completely random bucket.
  // num_buckets: the total number of possible input values (which is equal to
  //   the total number of possible output values).
  // estimate_type: kPessimistic denoting that the rounding is done in such a
  //   way that the resulting epsilon-hockey stick divergence computation gives
  //   an upper estimate to the real value.
  static absl::StatusOr<std::unique_ptr<PrivacyLossDistribution>>
  CreateForRandomizedResponse(
      double noise_parameter, int num_buckets,
      EstimateType estimate_type = EstimateType::kPessimistic,
      double discretization_interval = 1e-4);

  // Creates {@link PrivacyLossDistribution} for the Laplace mechanism.
  //
  // parameter: the parameter of the Laplace distribution.
  // sensitivity: the sensitivity of function f. (i.e. the maximum absolute
  //   change in f when an input to a single user changes.)
  // estimate_type: kPessimistic denoting that the rounding is done in such a
  //   way that the resulting epsilon-hockey stick divergence computation gives
  //   an upper estimate to the real value.
  // discretization_interval: the length of the dicretization interval for the
  //   privacy loss distribution. The values will be rounded up/down to be
  //   integer multiples of this number.
  static absl::StatusOr<std::unique_ptr<PrivacyLossDistribution>>
  CreateForLaplaceMechanism(
      double parameter, double sensitivity = 1,
      EstimateType estimate_type = EstimateType::kPessimistic,
      double discretization_interval = 1e-4);

  // Creates {@link PrivacyLossDistribution} for the Discrete Laplace mechanism.
  //
  // parameter: the parameter of the Discrete Laplace distribution.
  // sensitivity: the sensitivity of function f. (i.e. the maximum absolute
  //   change in f when an input to a single user changes.)
  // estimate_type: kPessimistic denoting that the rounding is done in such a
  //   way that the resulting epsilon-hockey stick divergence computation gives
  //   an upper estimate to the real value.
  // discretization_interval: the length of the dicretization interval for the
  //   privacy loss distribution. The values will be rounded up/down to be
  //   integer multiples of this number.
  static absl::StatusOr<std::unique_ptr<PrivacyLossDistribution>>
  CreateForDiscreteLaplaceMechanism(
      double parameter, int sensitivity = 1,
      EstimateType estimate_type = EstimateType::kPessimistic,
      double discretization_interval = 1e-4);

  // Creates {@link PrivacyLossDistribution} for the Gaussian mechanism.
  //
  // standard_deviation: the standard_deviation of the Gaussian distribution.
  // sensitivity: the sensitivity of function f. (i.e. the maximum absolute
  //   change in f when an input to a single user changes.)
  // estimate_type: kPessimistic denoting that the rounding is done in such a
  //   way that the resulting epsilon-hockey stick divergence computation gives
  //   an upper estimate to the real value.
  // discretization_interval: the length of the dicretization interval for the
  //   privacy loss distribution. The values will be rounded up/down to be
  //   integer multiples of this number.
  // mass_truncation_bound: the natural log of the probability mass that might
  //   be discarded from the noise distribution. The larger this number, the
  //   more error it may introduce in divergence calculations.
  static absl::StatusOr<std::unique_ptr<PrivacyLossDistribution>>
  CreateForGaussianMechanism(
      double standard_deviation, double sensitivity = 1,
      EstimateType estimate_type = EstimateType::kPessimistic,
      double discretization_interval = 1e-4,
      double mass_truncation_bound = -50);

  // Creates {@link PrivacyLossDistribution} for the Gaussian mechanism.
  //
  // sigma: he parameter of the discrete Gaussian distribution. Note that unlike
  //   the (continuous) Gaussian distribution this is not equal to the standard
  //   deviation of the noise.
  // sensitivity: the sensitivity of function f. (i.e. the maximum absolute
  //   change in f when an input to a single user changes.)
  // estimate_type: kPessimistic denoting that the rounding is done in such a
  //   way that the resulting epsilon-hockey stick divergence computation gives
  //   an upper estimate to the real value.
  // discretization_interval: the length of the dicretization interval for the
  //   privacy loss distribution. The values will be rounded up/down to be
  //   integer multiples of this number.
  // mass_truncation_bound: the natural log of the probability mass that might
  //   be discarded from the noise distribution. The larger this number, the
  //   more error it may introduce in divergence calculations.
  // truncation_bound: bound for truncating the noise, i.e. the noise will only
  //   have a support in [-truncation_bound, truncation_bound]. When not set,
  //   truncation_bound will be chosen in such a way that the mass of the noise
  //   outside of this range is at most 1e-30.
  static absl::StatusOr<std::unique_ptr<PrivacyLossDistribution>>
  CreateForDiscreteGaussianMechanism(
      double sigma, int sensitivity = 1,
      EstimateType estimate_type = EstimateType::kPessimistic,
      double discretization_interval = 1e-4,
      std::optional<int> truncation_bound = std::nullopt);

  // Creates {@link PrivacyLossDistribution} from epsilon and delta parameters.
  //
  // When the mechanism is (epsilon, delta)-differentially private, the
  // following is a pessimistic estimate of its privacy loss distribution (see
  // Section 3.5 of the supplementary material for more explanation):
  //  - infinity with probability delta.
  //  - epsilon with probability (1 - delta) / (1 + exp(-eps))
  //  - -epsilon with probability (1 - delta) / (1 + exp(eps))
  //
  // privacy_parameters: the privacy guarantee of the mechanism.
  static std::unique_ptr<PrivacyLossDistribution> CreateForPrivacyParameters(
      EpsilonDelta epsilon_delta, double discretization_interval = 1e-4);

  PrivacyLossDistribution& operator=(const PrivacyLossDistribution&) = delete;

  // Computes the epsilon-hockey stick divergence between mu_upper, mu_lower.
  //
  // When this privacy loss distribution corresponds to a mechanism, the
  // epsilon-hockey stick divergence gives the value of delta for which the
  // mechanism is (epsilon, delta)-differentially private. (See Observation 1 in
  // the supplementary material.)
  double GetDeltaForEpsilon(double epsilon) const;

  // Computes the smallest non-negative epsilon for which hockey stick
  // divergence is at most delta. When no such finite epsilon exists, return
  // std::numeric_limits<double>::infinity().
  //
  // When this privacy loss distribution corresponds to a mechanism and the
  // rounding is pessimistic, the returned value corresponds to an epsilon for
  // which the mechanism is (epsilon, delta)-differentially private. (See
  // Observation 1 in the supplementary material.)
  double GetEpsilonForDelta(double delta) const;

  // Validates that a given PLD can be composed with this PLD. The
  // discretization intervals and the estimate types should be the same;
  // otherwise failure status is returned.
  absl::Status ValidateComposition(
      const PrivacyLossDistribution& other_pld) const;

  // Composes other PLD into itself. Additional parameter:
  //   tail_mass_truncation: an upper bound on the tails of the probability
  //     mass of the PLD that might be truncated.
  absl::Status Compose(const PrivacyLossDistribution& other_pld,
                       double tail_mass_truncation = 1e-15);

  // Computes delta for given epsilon for the result of composing this PLD and a
  // given PLD. Note that this function does not modify the current PLD.
  //
  // The output of this function should be the same as first composing this PLD
  // and other_pld, and then call GetEpsilonForDelta on the resulting
  // PLD. The main advantage is that this function is faster.
  absl::StatusOr<double> GetDeltaForEpsilonForComposedPLD(
      const PrivacyLossDistribution& other_pld, double epsilon) const;

  // Composes PLD into itself num_times. Additional parameter:
  //   tail_mass_truncation: an upper bound on the tails of the probability mass
  //     of the PLD that might be truncated. Currently only supports for
  //     pessimistic estimates.
  void Compose(int num_times, double tail_mass_truncation = 1e-15);

  double DiscretizationInterval() const { return discretization_interval_; }

  EstimateType GetEstimateType() const { return estimate_type_; }

  //  The probability mass of mu_upper over all the outcomes that
  //  can occur only in mu_upper but not in mu_lower. (These outcomes result in
  //  privacy loss ln(mu_upper(o) / mu_lower(o)) of infinity.)
  double InfinityMass() const { return infinity_mass_; }
  const ProbabilityMassFunction& Pmf() const {
    return probability_mass_function_;
  }

  // Serializes the privacy loss distribution. Currently only supports
  // pessimistic estimates.
  absl::StatusOr<serialization::PrivacyLossDistribution> Serialize() const;

  // Deserializes the privacy loss distribution.
  static absl::StatusOr<std::unique_ptr<PrivacyLossDistribution>> Deserialize(
      const serialization::PrivacyLossDistribution& proto);

 private:
  PrivacyLossDistribution(
      double discretization_interval, double infinity_mass,
      const ProbabilityMassFunction& probability_mass_function,
      EstimateType estimate_type = EstimateType::kPessimistic)
      : discretization_interval_(discretization_interval),
        infinity_mass_(infinity_mass),
        probability_mass_function_(probability_mass_function),
        estimate_type_(estimate_type) {}

  const double discretization_interval_;
  double infinity_mass_;
  ProbabilityMassFunction probability_mass_function_;
  const EstimateType estimate_type_;

  friend class PrivacyLossDistributionTestPeer;
};
}  // namespace accounting
}  // namespace differential_privacy

#endif  // DIFFERENTIAL_PRIVACY_ACCOUNTING_PRIVACY_LOSS_DISTRIBUTION_H_
