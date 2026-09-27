# Convenience targets that run across all four activities.
ACTIVITIES := mp1-castle mp2-mountain mp3-cubes pe1-castle-3d

.PHONY: submission clean

# Build every activity's submission/ folder (see each activity's Makefile)
submission:
	@for activity in $(ACTIVITIES); do \
		echo "== $$activity"; \
		$(MAKE) --no-print-directory -C $$activity submission || exit 1; \
	done

clean:
	@for activity in $(ACTIVITIES); do $(MAKE) --no-print-directory -C $$activity clean; done
