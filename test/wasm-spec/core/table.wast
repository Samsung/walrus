;; Test table section structure

(module (table 0 funcref))
(module (table 1 funcref))
(module (table 0 0 funcref))
(module (table 0 1 funcref))
(module (table 1 256 funcref))
(module (table 0 65536 funcref))
(module (table 0 0xffff_ffff funcref))

(module (table 0 funcref) (table 0 funcref))
(module (table (import "spectest" "table") 0 funcref) (table 0 funcref))
