#!/opt/local/bin/perl -w

sub getFStr($) {
    my $f = shift;

    if ($f eq "0x1") {
        $fStr = "RF";
    } elsif ($f eq "0x2") {
        $fStr = "ID";
    } elsif ($f eq "0x4") {
        $fStr = "MDU";
    } elsif ($f eq "0x8") {
        $fStr = "MEM_PRE_A";
    } elsif ($f eq "0x10") {
        $fStr = "MEM_POST";
    } elsif ($f eq "0x20") {
        $fStr = "WB";
    } elsif ($f eq "0x40") {
        $fStr = "ALU";
    } elsif ($f eq "0x80") {
        $fStr = "MEM_BP";
    } elsif ($f eq "0x100") {
        $fStr = "CONTROL";
    } elsif ($f eq "0x200") {
        $fStr = "INST_ADDR";
    } elsif ($f eq "0x400") {
        $fStr = "INST_TYPE";
    } elsif ($f eq "0x800") {
        $fStr = "WB_ADDR";
    } elsif ($f eq "0x1000") {
        $fStr = "MEM_PRE_D";
    } elsif ($f eq "0x2000") {
        $fStr = "PC";
    } else {
        $fStr = "Unknown Fault". $f;
    }

    return $fStr;
}

sub calcFailDist($$$$) {
    my ($line, $injPoint, $execFile, $faultLoc) = @_;
    my @ws = split(/ /,$line);
    my $failPoint = $ws[-1];
    my $failDist = $failPoint - $injPoint;
    # need to store
    #printf("fail d %d\n", $failDist);
}


@resCat = ("SDC", "terminated", "Timeout", "CORRECT");


my $dir ="./out";
my $numP = 0;
foreach my $fp (glob("$dir/sstOut*")) {
  printf "processing %s (#%d) found: ", $fp, ++$numP;

  @fp_w = split(/-/,$fp);
  $fp_w[1] =~ s/\..*//;  #remove end
  $execFile = $fp_w[1];
  $isQSort = ($execFile =~ /qsort/);
  printf(" exec:%s Qs:%d\n", $execFile, $isQSort);

  $done = 0;
  $injPoint = -1;
  my $numFound = 0;
  my $debug = 0;  # for debuging results if numFound / somFound don't
                  # match


  open my $fh, "<", $fp or die "can't read open '$fp': $OS_ERROR";
  while (defined($line = <$fh>)) {
      chop($line);

      if (!$done && $line =~ /PRINT_INT/) {
          @ws = split(/ /,$line);
          if ($ws[1] != 0) {
              #SDC detected
              $results{$execFile}{$faultLoc}{"SDC"}++;
          } else {
              #finished fine
              $results{$execFile}{$faultLoc}{"CORRECT"}++;
          }
          if ($debug) {printf("C");}
          $done = 1;  # mark done so we only count once
      } elsif (!$done && $line =~ /INJECTING/) {
          @ws = split(/ /,$line);
          if (!($ws[-1] =~ /effect/)) {  # avoid 'INJECTING fault to
                                         # data on load: no effect'
              $injPoint = $ws[-1];
          }
      } elsif (!$done && $line =~ /UNRECOVERABLE ERROR: 65280/) {
          # special case, probably caused by INST_TYPE
          $results{$execFile}{$faultLoc}{"terminated"}++;
          if ($debug) {printf("U");}
          if ($done) {printf("X\n");}
      } elsif (!$done && $line =~ /invalid instruction/) {
          calcFailDist($line, $injPoint, $execFile, $faultLoc);
          $results{$execFile}{$faultLoc}{"terminated"}++;
          if ($debug) {printf("T");}
          if ($done) {printf("X\n");}
      } elsif (!$done && $line =~ /Unknown ex/) {
          calcFailDist($line, $injPoint, $execFile, $faultLoc);
          $results{$execFile}{$faultLoc}{"terminated"}++;
          if ($debug) {printf("T");}
          if ($done) {printf("X\n");}
      } elsif (!$done && $line =~ /terminated/) {
          calcFailDist($line, $injPoint, $execFile, $faultLoc);
          $results{$execFile}{$faultLoc}{"terminated"}++;
          if ($debug) {printf("T");}
          if ($done) {printf("X\n");}
      } elsif (!$done && $line =~ /Timeout/) {
          $results{$execFile}{$faultLoc}{"Timeout"}++;
          if ($debug) {printf("TO");}
          if ($done) {printf("X\n");}
      } elsif ($line =~ /CORRECTED_MATH/) {
          @ws = split(/ /,$line);
          if ($ws[2] != 0) {
              # at least 1 math correction
              $results{$execFile}{$faultLoc}{"MthCs"}++;
              $results{$execFile}{$faultLoc}{"#MthC"} += $ws[2];
          }
      } elsif ($line =~ /WB_ERROR/) {
          @ws = split(/ /,$line);
          if ($ws[2] != 0) {
              # at least 1 WB error correction
              $results{$execFile}{$faultLoc}{"WBEs"}++;
              $results{$execFile}{$faultLoc}{"#WBE"} += $ws[2];
          }
      } elsif ($line =~ /-----------/) {
          $line =~ s/--*/-/g;
          @ws = split(/-/,$line);
          $faultLoc = $ws[1];
          if ($debug) {printf("\n $faultLoc ");}
          # next run
          $done = 0;
          $injPoint = -1;
          $numFound++;
      }

  }

  my $sumFound = 0;
  foreach my $r (@resCat) {
      foreach my $fl ("0x1", "0x2", "0x4", "0x8",
                      "0x10", "0x20", "0x40", "0x80",
                      "0x100", "0x200", "0x400", "0x800",
                      "0x1000", "0x2000") {
          if(defined($results{$execFile}{$fl}{$r})) {
              $sumFound += $results{$execFile}{$fl}{$r};
          }
      }
  }
  printf(" %d / %d\n", $numFound, $sumFound);
  close $fh or die "can't read close '$fp': $OS_ERROR";
}

@statCat = ("MthCs", "#MthC", "WBEs", "#WBE");
@statPer = (1,0,1,0);  # print as % of runs
$printPercent = 1; # print any as percents

printf("                                ");
printf("%7s\t\t", "runs");
foreach $r (@resCat, @statCat) {
    printf("%7s\t", substr($r,0,6));
}
printf("\n");

foreach $e (sort keys %results) {
    foreach $f (sort keys %{$results{$e}}) {

        $fStr = getFStr($f);


        $runs{$e} = 0;
        foreach $r (@resCat) {
            if (defined($results{$e}{$f}{$r})) {
                $runs{$e} += $results{$e}{$f}{$r};
            }
        }

        $nameStr = $e;
        $nameStr =~ s/matmat//;
        $nameStr =~ s/_//;
        if ($nameStr eq "") {
            $nameStr = "matmat";
        } elsif ($nameStr eq "O3") {
            $nameStr = "matmatO3";
        }
        printf("%10s\t%10s\t%6d\t\t", substr($nameStr,0,10), $fStr, $runs{$e});

        foreach $r (@resCat) {
            if (defined($results{$e}{$f}{$r})) {
                if ($printPercent) {
                    printf("%6.2f%%\t", $results{$e}{$f}{$r}*100.0/$runs{$e});
                } else {
                    printf("%6.0f\t", $results{$e}{$f}{$r});
                }
            } else {
                if ($printPercent) {
                    printf("%6.2f%%\t", 0);
                } else {
                    printf("%6.0f\t", 0);
                }
            }
        }

        $i = 0;
        foreach $r (@statCat) {            
            if (defined($results{$e}{$f}{$r})) {
                if ($statPer[$i] & $printPercent) {
                    printf("%6.2f%%\t", $results{$e}{$f}{$r}*100.0/$runs{$e});
                } else {
                    printf("%6.3f\t", $results{$e}{$f}{$r}/$runs{$e});
                }
            } else {
                if ($statPer[$i] & $printPercent) {
                    printf("%6.2f%%\t", 0);
                } else {
                    printf("%6.0f\t", 0);
                }
            }
            $i++;
        }

        printf("\n");
    }
}
