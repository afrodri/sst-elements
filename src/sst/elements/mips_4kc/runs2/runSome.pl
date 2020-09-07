#!/opt/local/bin/perl -w

# ./runSome.pl <execFile> <outputFile>


# name is the fault location and 'D' for data, 'C' for control, 'I'
# for special INST_ADDR case
%probabilities = (    
    "0x40D" => 64, #ALU	64
    #"" => , #CONTROL	52
    "0x200I" => 8, #INST_ADDR	8
    "0x400D" => 32, #INST_TYPE - data	32
    "0x400C" => 5, #INST_TYPE - control	5
    "0x4D" => 65, #MDU - data	65
    "0x4C" => 14, #MDU - control	14
    "0x10D" => 32, #MEM_POST -data	32
    "0x10C" => 3, #MEM_POST - control	3
    "0x8D" => 59, #MEM_PRE_ADDR -data	59
    "0x1000D" => 32, #MEM_PRE_DATA -data	32
    "0x1D" => 992, #RF data	992
    "0x800D" => 5, #RF control (WB_ADDR)	5
    "0x80D" => 32, #MEM_bp_val -data	32
    "0x20D" => 32 # WB 32
    );
$maxChance = 0;

#initialize the probability table
sub initProb() {
    my $sum = 0;
    foreach my $k (sort keys %probabilities) {
        #printf("%s: %d ", $k, $probabilities{$k});
        $sum += $probabilities{$k};
        $probabilities{$k} = $sum;
        #printf("%d\n", $probabilities{$k});
    }
    $maxChance = $sum+1;
}

sub getFault() {
    my $roll = int(rand($maxChance));
    my $f = "0x1";  # fault type
    $b = "0"; # bits to flip (0= choose one random)

    # see where we fall
    foreach my $k (sort keys %probabilities) {
        if ($roll <= $probabilities{$k}) {
            $f = $k;
            last;
        }
    }

    #determine which bits to flip
    my $type = chop($f);
    if ($type eq 'I') {
        # pick one bit in 0b1111111100
        my $roll2 = int(rand(8));
        my $b = 1 << ($roll2 + 2);
        #printf("I $roll2 %x\n", $b);
    } elsif ($type eq 'D') {
        $b = 0;
    } elsif ($type eq 'C') {
        $b = int(rand(0xffffffff));
    } else {
        printf("Bad bit flip type: $f $type $roll !!!!!!!\n");
        exit(-1);
    }


    
    return ($f, $b);
}

#"main"
$i = 0;
$e = $ARGV[0];
$file = $ARGV[1];
srand();
initProb();
while ($i < 100) {
    my ($f, $b) = getFault();
    $str = "sst ../test_by_time.py -- -e $e -f $f -b $b";
    if (0) { # test
        printf("$str\n");
    } else {
        system("echo \"---------------------$f-$b-$e\" 1>>${file} 2>&1");
        system("${str} 1>>${file} 2>&1");
    }
    $i++;
}


