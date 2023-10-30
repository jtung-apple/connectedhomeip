/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

package matter.devicecontroller.cluster.clusters

import java.util.ArrayList

class OtaSoftwareUpdateRequestorCluster(private val endpointId: UShort) {
  class DefaultOTAProvidersAttribute(
    val value: ArrayList<ChipStructs.OtaSoftwareUpdateRequestorClusterProviderLocation>
  )

  class UpdateStateProgressAttribute(val value: UByte?)

  class GeneratedCommandListAttribute(val value: ArrayList<UInt>)

  class AcceptedCommandListAttribute(val value: ArrayList<UInt>)

  class EventListAttribute(val value: ArrayList<UInt>)

  class AttributeListAttribute(val value: ArrayList<UInt>)

  suspend fun announceOTAProvider(
    providerNodeID: ULong,
    vendorID: UShort,
    announcementReason: UInt,
    metadataForNode: ByteArray?,
    endpoint: UShort
  ) {
    // Implementation needs to be added here
  }

  suspend fun announceOTAProvider(
    providerNodeID: ULong,
    vendorID: UShort,
    announcementReason: UInt,
    metadataForNode: ByteArray?,
    endpoint: UShort,
    timedInvokeTimeoutMs: Int
  ) {
    // Implementation needs to be added here
  }

  suspend fun readDefaultOTAProvidersAttribute(): DefaultOTAProvidersAttribute {
    // Implementation needs to be added here
  }

  suspend fun readDefaultOTAProvidersAttributeWithFabricFilter(
    isFabricFiltered: Boolean
  ): DefaultOTAProvidersAttribute {
    // Implementation needs to be added here
  }

  suspend fun writeDefaultOTAProvidersAttribute(
    value: ArrayList<ChipStructs.OtaSoftwareUpdateRequestorClusterProviderLocation>
  ) {
    // Implementation needs to be added here
  }

  suspend fun writeDefaultOTAProvidersAttribute(
    value: ArrayList<ChipStructs.OtaSoftwareUpdateRequestorClusterProviderLocation>,
    timedWriteTimeoutMs: Int
  ) {
    // Implementation needs to be added here
  }

  suspend fun subscribeDefaultOTAProvidersAttribute(
    minInterval: Int,
    maxInterval: Int
  ): DefaultOTAProvidersAttribute {
    // Implementation needs to be added here
  }

  suspend fun readUpdatePossibleAttribute(): Boolean {
    // Implementation needs to be added here
  }

  suspend fun subscribeUpdatePossibleAttribute(minInterval: Int, maxInterval: Int): Boolean {
    // Implementation needs to be added here
  }

  suspend fun readUpdateStateAttribute(): Integer {
    // Implementation needs to be added here
  }

  suspend fun subscribeUpdateStateAttribute(minInterval: Int, maxInterval: Int): Integer {
    // Implementation needs to be added here
  }

  suspend fun readUpdateStateProgressAttribute(): UpdateStateProgressAttribute {
    // Implementation needs to be added here
  }

  suspend fun subscribeUpdateStateProgressAttribute(
    minInterval: Int,
    maxInterval: Int
  ): UpdateStateProgressAttribute {
    // Implementation needs to be added here
  }

  suspend fun readGeneratedCommandListAttribute(): GeneratedCommandListAttribute {
    // Implementation needs to be added here
  }

  suspend fun subscribeGeneratedCommandListAttribute(
    minInterval: Int,
    maxInterval: Int
  ): GeneratedCommandListAttribute {
    // Implementation needs to be added here
  }

  suspend fun readAcceptedCommandListAttribute(): AcceptedCommandListAttribute {
    // Implementation needs to be added here
  }

  suspend fun subscribeAcceptedCommandListAttribute(
    minInterval: Int,
    maxInterval: Int
  ): AcceptedCommandListAttribute {
    // Implementation needs to be added here
  }

  suspend fun readEventListAttribute(): EventListAttribute {
    // Implementation needs to be added here
  }

  suspend fun subscribeEventListAttribute(minInterval: Int, maxInterval: Int): EventListAttribute {
    // Implementation needs to be added here
  }

  suspend fun readAttributeListAttribute(): AttributeListAttribute {
    // Implementation needs to be added here
  }

  suspend fun subscribeAttributeListAttribute(
    minInterval: Int,
    maxInterval: Int
  ): AttributeListAttribute {
    // Implementation needs to be added here
  }

  suspend fun readFeatureMapAttribute(): Long {
    // Implementation needs to be added here
  }

  suspend fun subscribeFeatureMapAttribute(minInterval: Int, maxInterval: Int): Long {
    // Implementation needs to be added here
  }

  suspend fun readClusterRevisionAttribute(): Integer {
    // Implementation needs to be added here
  }

  suspend fun subscribeClusterRevisionAttribute(minInterval: Int, maxInterval: Int): Integer {
    // Implementation needs to be added here
  }

  companion object {
    const val CLUSTER_ID: UInt = 42u
  }
}