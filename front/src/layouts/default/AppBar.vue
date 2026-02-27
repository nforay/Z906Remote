<template>
  <v-app-bar :elevation="2" title="Z906 Remote">
    <template v-slot:append>
      <div class="d-flex align-center ga-2">
        <template v-if="versionStore.updateAvailable">
          <v-btn
          :href="`https://github.com/nforay/Z906Remote/releases/tag/${versionStore.latestVersion}`"
          target="_blank"
          variant="tonal"
          size="x-small"
          color="success"
          class="ml-2"
          >
          <v-icon size="14" class="mr-1">
            $tag
          </v-icon>
            {{ versionStore.latestVersion }}
          </v-btn>
        </template>
        <v-tooltip text="Star me on GitHub 😉">
          <template #activator="{ props }">
            <v-badge
              :model-value="versionStore.updateAvailable"
              location="top right"
              color="success"
              dot
              offset-x="8"
              offset-y="8"
            >
              <v-btn
                v-bind="props"
                icon
                href="https://github.com/nforay/Z906Remote"
                target="_blank"
              >
                <v-icon size="36">$github</v-icon>
              </v-btn>
            </v-badge>
          </template>
        </v-tooltip>
      </div>
    </template>
  </v-app-bar>
</template>

<script lang="ts" setup>
import { useVersionStore } from "@/stores/VersionStore"

const versionStore = useVersionStore()
</script>
