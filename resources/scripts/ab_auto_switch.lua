-- Auto A/B switcher — wire ScriptOut -> the A/B Select node's data-in port.
-- Rename that node's connected slots to "Camera A" / "Camera B" (double-click
-- the slot name). Add a Trigger node (Interval) into this script's DataIn and
-- set this script's trigger to "On input change", then it alternates both decks
-- every 5 seconds. Targets can be a slot name or a 1-based index,
-- e.g. return { a = 2, b = 1 }.
if os.time() % 10 < 5 then
  return { a = "Camera A", b = "Camera B" }
else
  return { a = "Camera B", b = "Camera A" }
end
