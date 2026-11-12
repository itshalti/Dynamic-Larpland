#include "island/ui_palette.h"

#include "island/theme.h"
#include "ui/palette.h"

void applyUiPalette(const WallpaperService& wallpapers) {
  const Color accent = theme::accentColor(wallpapers);
  const Color onAccent = theme::accentText(wallpapers);
  setPalette(Palette{
      .primary = accent,
      .onPrimary = onAccent,
      .primaryContainer = theme::surfaceHover,
      .onPrimaryContainer = theme::textPrimary,
      .secondary = accent,
      .onSecondary = onAccent,
      .secondaryContainer = theme::surfaceRaised,
      .onSecondaryContainer = theme::textPrimary,
      .tertiary = theme::weekend,
      .onTertiary = theme::textPrimary,
      .tertiaryContainer = theme::surfaceRaised,
      .onTertiaryContainer = theme::textPrimary,
      .error = theme::recording,
      .onError = theme::textPrimary,
      .errorContainer = theme::surfaceRaised,
      .onErrorContainer = theme::recording,
      .surface = theme::surface,
      .onSurface = theme::textPrimary,
      .onSurfaceVariant = theme::textSecondary,
      .surfaceContainerLow = theme::surface,
      .surfaceContainer = theme::surfaceRaised,
      .surfaceContainerHigh = theme::surfaceRaised,
      .surfaceContainerHighest = theme::surfaceHover,
      .outlineVariant = theme::divider,
      .shadow = theme::surface,
      .scrim = Color{0.0F, 0.0F, 0.0F, 0.5F},
      .hover = theme::surfaceHover,
      .onHover = theme::textPrimary,
  });
}
