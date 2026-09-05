#!/usr/bin/env python3
from __future__ import annotations

from collections import defaultdict
from typing import Any


class RasterVisualPolicy:
    """Candidate policy for the source-rebuild pilot.

    The policy is deliberately limited to raster PDF image objects. It does not
    alter text extraction and it does not attempt vector fallback: vector
    grouping needs its own validated pass before the full corpus is rebuilt.
    """

    def __init__(
        self,
        pe: Any,
        *,
        header_ratio: float = 0.075,
        repeated_small_min_pages: int = 5,
        repeated_small_max_dimension: float = 120.0,
        repeated_small_max_area_ratio: float = 0.02,
    ) -> None:
        self.pe = pe
        self.header_ratio = float(header_ratio)
        self.repeated_small_min_pages = int(repeated_small_min_pages)
        self.repeated_small_max_dimension = float(repeated_small_max_dimension)
        self.repeated_small_max_area_ratio = float(repeated_small_max_area_ratio)
        self._xref_page_count_cache: dict[int, dict[int, int]] = {}

    def _xref_page_counts(self, document: Any) -> dict[int, int]:
        cache_key = id(document)
        cached = self._xref_page_count_cache.get(cache_key)
        if cached is not None:
            return cached

        pages_by_xref: defaultdict[int, set[int]] = defaultdict(set)
        for page_number in range(int(document.page_count)):
            page = document[page_number]
            for image in page.get_images(full=True):
                xref = int(image[0])
                if xref <= 0:
                    continue
                try:
                    rects = page.get_image_rects(xref)
                except Exception:
                    rects = []
                if rects:
                    pages_by_xref[xref].add(page_number)

        result = {xref: len(pages) for xref, pages in pages_by_xref.items()}
        self._xref_page_count_cache[cache_key] = result
        return result

    def candidate_rects(self, page: Any) -> list[Any]:
        page_area = max(1.0, float(page.rect.width * page.rect.height))
        header_limit = float(page.rect.height) * self.header_ratio
        document = page.parent
        xref_page_counts = self._xref_page_counts(document)

        candidates: list[Any] = []
        seen: set[tuple[int, int, int, int]] = set()
        for image in page.get_images(full=True):
            xref = int(image[0])
            if xref <= 0:
                continue
            try:
                rects = page.get_image_rects(xref)
            except Exception:
                rects = []
            for raw in rects:
                rect = self.pe.fitz.Rect(raw) & page.rect
                if rect.is_empty or rect.width < 18 or rect.height < 18:
                    continue
                area_ratio = float(rect.width * rect.height) / page_area
                if area_ratio < 0.003:
                    continue

                key = tuple(int(round(v * 2.0)) for v in rect)
                if key in seen:
                    continue
                seen.add(key)

                # Workshop-manual header exclusion applies only to visual
                # candidates. Text/header reading remains untouched.
                if float(rect.y1) <= header_limit:
                    continue

                # Repeated small raster resources are section/navigation
                # pictograms in RCL0193ENG. Keep large repeated technical
                # illustrations eligible for review.
                if (
                    xref_page_counts.get(xref, 0) >= self.repeated_small_min_pages
                    and max(float(rect.width), float(rect.height)) <= self.repeated_small_max_dimension
                    and area_ratio <= self.repeated_small_max_area_ratio
                ):
                    continue

                candidates.append(rect)

        # No vector fallback in this pilot. The historical page-wide union is
        # intentionally not reused because it can merge unrelated drawings.
        return candidates

    def exact_raster_clip(self, rect: Any, page_rect: Any, margin: float = 0.0) -> Any:
        del margin
        clip = self.pe.fitz.Rect(rect) & page_rect
        return clip


def install_source_rebuild_visual_policy(
    pe: Any,
    *,
    header_ratio: float = 0.075,
    repeated_small_min_pages: int = 5,
    repeated_small_max_dimension: float = 120.0,
    repeated_small_max_area_ratio: float = 0.02,
) -> RasterVisualPolicy:
    policy = RasterVisualPolicy(
        pe,
        header_ratio=header_ratio,
        repeated_small_min_pages=repeated_small_min_pages,
        repeated_small_max_dimension=repeated_small_max_dimension,
        repeated_small_max_area_ratio=repeated_small_max_area_ratio,
    )
    pe.visual_candidate_rects = policy.candidate_rects
    pe.expanded_clip = policy.exact_raster_clip
    return policy
