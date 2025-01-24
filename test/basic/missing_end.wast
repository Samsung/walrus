(assert_invalid
  (module binary
    "\00asm" "\01\00\00\00"
    "\01\09"                      ;; import section
    "\02\60\00\00\60\01\7f\01\7f" ;; type section entry
    "\03\02"                      ;; function section
    "\01\00"                      ;; function section entry
    "\07\09"                      ;; export section
    "\01\05\6e\6f\65\6e\64\00\00" ;; export section entry (function 'noend')
    "\0a\18"                      ;; begin code section
    "\01\16\00"                   ;; begin function body
    "\02\40"                      ;; block
    "\03\40"                      ;;   loop
    "\02\40"                      ;;     block
    "\03\40"                      ;;       loop
    "\0c\00"                      ;;         br 0
    "\02\01"                      ;;         block type[1]
    "\0b"                         ;;         end
    "\0c\00"                      ;;         br 0
    "\0b"                         ;;       end
    "\00"                         ;;       unreachable
    "\00"                         ;;       unreachable
    "\00"                         ;;       unreachable
    "\0b"                         ;;     end
    "\0b"                         ;;   end
  )
  "function body must end with END opcode"
)
