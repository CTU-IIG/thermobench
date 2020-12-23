;;; Directory Local Variables
;;; For more information see (info "(emacs) Directory Variables")

((markdown-mode
  . ((eval . (add-hook 'after-save-hook 'wsh/thermobench\.jl-make-doc nil t)))))
