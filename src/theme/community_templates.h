#pragma once

#include "theme/builtin_templates.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

class HttpClient;

namespace noctalia::theme {

  // The subset of Noctalia's ThemeConfig::TemplatesConfig the service reads.
  struct CommunityTemplatesConfig {
    bool enableCommunityTemplates = true;
    std::vector<std::string> communityIds;
  };

  class CommunityTemplateService {
  public:
    using ReadyCallback = std::function<void()>;

    explicit CommunityTemplateService(HttpClient& httpClient);

    void setReadyCallback(ReadyCallback callback);
    void sync(const CommunityTemplatesConfig& templates);

    static std::vector<AvailableTemplate> availableTemplates();

  private:
    void syncSelectedFromCatalog(
        const std::vector<std::string>& selectedIds, std::uint64_t generation, bool notifyWhenReady
    );

    void syncCatalogManifests(const std::vector<std::string>& selectedIds, std::uint64_t generation);
    HttpClient& m_httpClient;
    ReadyCallback m_readyCallback;
    std::uint64_t m_generation = 0;
  };

  std::filesystem::path communityTemplatesCacheDir();
  std::filesystem::path communityTemplateDir(std::string_view id);
  std::filesystem::path communityTemplateConfigPath(std::string_view id);
  bool isSafeCommunityTemplateId(std::string_view id);

} // namespace noctalia::theme
