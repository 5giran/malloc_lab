#!/usr/bin/perl 
#!/usr/local/bin/perl 
use Getopt::Std;

#######################################################################
# checktrace - trace file 일관성 검사 및 balance 조정 도구.
#
# Copyright (c) 2002, R. Bryant and D. O'Hallaron, 모든 권리 보유.
# 허가 없이 사용, 수정, 복사할 수 없습니다.
#
# 이 스크립트는 Malloc Lab trace file을 읽어 일관성을 검사하고,
# 필요한 free 요청을 덧붙여 balanced 버전을 출력합니다.
#
#######################################################################
 
$| = 1; # 모든 print 문마다 출력을 즉시 flush

#
# void usage(void) - 도움말을 출력하고 종료
#
sub usage 
{
    printf STDERR "$_[0]\n";
    printf STDERR "Usage: $0 [-hs]\n";
    printf STDERR "Options:\n";
    printf STDERR "  -h          Print this message\n";
    printf STDERR "  -s          Emit only a brief summary\n";
    die "\n" ;
}

##############
# 메인 루틴
##############

# 
# 명령줄 인자를 파싱하고 검사
#
getopts('hs');
if ($opt_h) {
    usage("");
}
$summary = $opt_s;

# 
# HASH는 아직 대응되지 않은 alloc/realloc 요청을 계속 집계합니다.
# free 요청을 만나면 해당 hash 항목을 삭제합니다.
# trace를 끝까지 읽고 남는 것은 짝이 없는 alloc/realloc 요청입니다.
#
%HASH = (); 

# trace header 값을 읽음
$heap_size = <STDIN>;
chomp($heap_size);

$num_blocks = <STDIN>;
chomp($num_blocks);

$old_num_ops = <STDIN>;
chomp($old_num_ops);

$weight = <STDIN>;
chomp($weight);

# 
# 대응되는 free 요청이 없는 allocate 요청을 찾음
#
$linenum = 4;
$requestnum = 0;
while ($line = <STDIN>) {
    chomp($line);
    $linenum++;

    ($cmd, $id, $size) = split(" ", $line);

    # 빈 줄은 무시
    if (!$cmd) {
	next;
    }

    # 나중에 출력할 수 있도록 줄을 저장
    $lines[$requestnum++] = $line;

    # 이전에 alloc 요청이 있었다면 realloc 요청은 그대로 허용
    if ($cmd eq "r") {
	if (!$HASH{$id}) {
	    die "$0: ERROR[$linenum]: realloc without previous alloc\n";
	}
	next;
    }

    if ($cmd eq "a" and $HASH{$id} eq "a") {
	die "$0: ERROR[$linenum]: allocate with no intervening free.\n";
    }

    if ($cmd eq "a" and $HASH{$id} eq "f") {
	die "$0: ERROR[$linenum]: reused ID $id.\n";
    }

    if ($cmd eq "f" and !exists($HASH{$id})) {
	die "$0: ERROR[$linenum]: freeing unallocated block.\n";
	next;
    }

    if ($cmd eq "f" and !$HASH{$id} eq "f") {
	die "$0: ERROR[$linenum]: freeing already freed block.\n";
	next;
    }
    
    if ($cmd eq "f") {
	delete $HASH{$id};
    }
    else {
	$HASH{$id} = $cmd;
    }
}

# 
# -s 인자와 함께 호출되면 간단한 balance 요약만 출력하고 종료
#
if ($summary) {
    if (!%HASH) {
	print "Balanced trace.\n";
    } 
    else {
	print "Unbalanced tree.\n";
    }
    exit;
}

#
# balanced된 trace 버전을 출력
#
$new_ops = keys %HASH;
$new_num_ops = $old_num_ops + $new_ops;

print "$heap_size\n";
print "$num_blocks\n";
print "$new_num_ops\n";
print "$weight\n";

# 기존 요청을 출력
foreach $item (@lines) {
    print "$item\n";
}

# trace를 balance시키는 free 요청 집합을 출력
foreach $key (sort keys %HASH) {
    if ($HASH{$key} ne "a" and $HASH{$key} ne "r") {
	die "$0: ERROR: Invalid free request in residue.\n";
    }
    print "f $key\n";
}

exit;
