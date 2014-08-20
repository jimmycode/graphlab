WITH RECURSIVE reachable(to_id) AS (
    SELECT to_id FROM p2p08 WHERE from_id = 1097
  UNION
    SELECT t.to_id
    FROM reachable r, p2p08 t
    WHERE r.to_id = t.from_id
  )
SELECT count(to_id) > 0
FROM reachable
WHERE to_id = 1208
