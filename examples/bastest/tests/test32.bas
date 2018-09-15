PUTS("abc")
END

SUB PUTS(s$)
  FOR i=1 to LEN(s$) : print mid$(s$,i,1); : NEXT
  PRINT
END SUB

