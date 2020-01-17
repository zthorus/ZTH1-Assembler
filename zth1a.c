/* Cross assembler for the ZTH1 CPU

  Date          Action
  ----          ------
  2019-11-14    Created
  2020-01-15    Corrected MAX_ISET and MAX_MSET
  2020-01-17    Increased number of labels (declarations and usages)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ISET 61
#define MAX_MSET 17

int getNumber8(char *s);
int getNumber16(char *s);
int readLine(FILE *f,char *s);
int writeFile(char *name,char **obj,char *mem,int b);

int main(int argc,char **argv)
{
  FILE *fsource;                /* source file */
  char iset[64][4];             /* instruction set */
  char decLabelTable[200][40];  /* table of declared labels */
  char fndLabelTable[800][40];  /* table of found labels */
  int labelValue[100];          /* table of declared label values */
  int fndLabelLoc[500];         /* table of found label locations */ 
  int val;                      /* value of a label */
  char *valStr;                 /* value (as string) of a label */
  int inString;           /* =1 if character string being read */
  int endLine;            /* =1 if end of line reached */
  int orgDef;             /* =1 if ORG has been defined */
  int instrProc;          /* =1 if instruction has been processed */
  int instrFound;         /* =1 if instruction has been found */
  int instrCode;          /* code of instruction */
  int instrArg;           /* argument (byte) of instruction */
  int labelMatch;         /* =1 if found label has a declared match */
  int compFailed;         /* =1 if compilation failed */
  int compDone;           /* =1 if compilation done (success or not) */
  char sLine[140];        /* line of source file being read */
  char instr[40];         /* instruction (or macro) being read */
  char hexVal[5];         /* value written in hexadecimal */
  char **objCode;         /* object code (list of 16-bit hexadecimal values) */
  char **objData;         /* object data (to be split into RAM_H and RAM_L) */
  char macroName[20][5];  /* names of macros */
  char macroCode[20][40]; /* codes of macros */ 
  int dlbl;               /* declared-label index */
  int flbl;               /* found-label index */
  int pc;                 /* program counter (mimic the PC register) */
  int it;                 /* instruction toggle (mimic the IT flag) */
  int ubytes;             /* number of useful bytes coded */
  int nops;               /* number of padding NOPs that have been inserted */
  float eff;              /* coding efficiency */
  char rom[8192];         /* words in ROM used */
  char ram[8192];         /* words in RAM used */ 
  int i,j,k,k0,m,n,st;

  if (argc != 2)
  {
    printf("syntax: zth1a <source file>\n");
    exit(0);
  }
  printf("Opening %s\n",argv[1]);
  fsource=fopen(argv[1],"r");
  if (fsource==NULL)
  {
    printf("Cannot open file: %s\n",argv[0]);
    exit(0);
  }
  
  strcpy(iset[0],"nop");
  strcpy(iset[1],"ldh");
  strcpy(iset[2],"ldl");
  strcpy(iset[3],"psh");
  strcpy(iset[4],"psl");
  strcpy(iset[5],"gth");
  strcpy(iset[6],"gtl");
  strcpy(iset[7],"gtw");
  strcpy(iset[8],"sth");
  strcpy(iset[9],"stl");
  strcpy(iset[10],"stw");
  strcpy(iset[11],"swa");
  strcpy(iset[12],"cll");
  strcpy(iset[13],"clh");
  strcpy(iset[14],"dup");
  strcpy(iset[15],"drp");
  strcpy(iset[16],"swp");
  strcpy(iset[17],"ru3");
  strcpy(iset[18],"ru4");
  strcpy(iset[19],"rd3");
  strcpy(iset[20],"rd4");
  strcpy(iset[21],"inc");
  strcpy(iset[22],"dec");
  strcpy(iset[23],"add");
  strcpy(iset[24],"sub");
  strcpy(iset[25],"and");
  strcpy(iset[26],"orr");
  strcpy(iset[27],"xor");
  strcpy(iset[28],"not");
  strcpy(iset[29],"neg");
  strcpy(iset[30],"ccf");
  strcpy(iset[31],"scf");
  strcpy(iset[32],"rrl");
  strcpy(iset[33],"rrw");
  strcpy(iset[34],"rll");
  strcpy(iset[35],"rlw");
  strcpy(iset[36],"btt");
  strcpy(iset[37],"cmp");
  strcpy(iset[38],"jmp");
  strcpy(iset[39],"jpz");
  strcpy(iset[40],"jnz");
  strcpy(iset[41],"jpc");
  strcpy(iset[42],"jnc");
  strcpy(iset[43],"cal");
  strcpy(iset[44],"clz");
  strcpy(iset[45],"cnz");
  strcpy(iset[46],"clc");
  strcpy(iset[47],"cnc");
  strcpy(iset[48],"ret");
  strcpy(iset[49],"rtz");
  strcpy(iset[50],"rnz");
  strcpy(iset[51],"rtc");
  strcpy(iset[52],"rnc");
  strcpy(iset[53],"eni");
  strcpy(iset[54],"dsi");
  strcpy(iset[55],"pu1");
  strcpy(iset[56],"pu2");
  strcpy(iset[57],"po1");
  strcpy(iset[58],"po2");
  strcpy(iset[59],"out");
  strcpy(iset[60],"inp");

  strcpy(macroName[0],"entr");
  strcpy(macroCode[0],"03000200");
  strcpy(macroName[1],"geth");
  strcpy(macroCode[1],"0300020005");
  strcpy(macroName[2],"getl");
  strcpy(macroCode[2],"0300020006");
  strcpy(macroName[3],"getw");
  strcpy(macroCode[3],"0300020007");
  strcpy(macroName[4],"comp");
  strcpy(macroCode[4],"03000200250F");
  strcpy(macroName[5],"jump");
  strcpy(macroCode[5],"0300020026");
  strcpy(macroName[6],"jmpz");
  strcpy(macroCode[6],"0300020027");
  strcpy(macroName[7],"jpnz");
  strcpy(macroCode[7],"0300020028");
  strcpy(macroName[8],"jmpc");
  strcpy(macroCode[8],"0300020029");
  strcpy(macroName[9],"jpnc");
  strcpy(macroCode[9],"030002002A");
  strcpy(macroName[10],"call");
  strcpy(macroCode[10],"030002002B");
  strcpy(macroName[11],"calz");
  strcpy(macroCode[11],"030002002C");
  strcpy(macroName[12],"clnz");
  strcpy(macroCode[12],"030002002D");
  strcpy(macroName[13],"calc");
  strcpy(macroCode[13],"030002002E");
  strcpy(macroName[14],"clnc");
  strcpy(macroCode[14],"030002002F");
  strcpy(macroName[15],"push");
  strcpy(macroCode[15],"3738");
  strcpy(macroName[16],"popp");
  strcpy(macroCode[16],"393A");

  /* problem if objCode and objData are declared with their dimensions
     when passed to writeFile function (segmentation fault) */
  objCode=(char **)malloc(8192*sizeof(char *));
  objData=(char **)malloc(8192*sizeof(char *));
  for (i=0;i<8192;i++)
  {
    rom[i]='0';
    ram[i]='0';
    objCode[i]=(char *)malloc(5*sizeof(char));
    objData[i]=(char *)malloc(5*sizeof(char));
  }
  compDone=0; 
  nops=0;
  ubytes=0;
  if (readLine(fsource,sLine)==0) compDone=1;
  compFailed=0;
  while (compDone==0)
  {
    printf("Line = %s\n",sLine);

    /* break line into instructions (; separation character) */
    st=0;
    endLine=0;
    if (strlen(sLine)==0) endLine=1;
    while (endLine==0)
    {
      j=0;
      inString=0;
      for (i=st;i<strlen(sLine);i++)
      {
        /* semi-columns allowed in constant strings (framed by ") */
        if (sLine[i]==34)
        {
          if (inString==0) inString=1; else inString=0;
        }
        if ((sLine[i]==';') && (inString==0))
        {
          instr[j]='\0';
          /* check if ; (optional) at end of line */
          if (i==(strlen(sLine-1))) endLine=1;
          break;
        }
        else
        {
          instr[j]=sLine[i];
          j++;
        }
      }
      if (i==strlen(sLine)) endLine=1;
      instr[j]='\0';
      st=i+1;

      /* parse the instruction */
     
      printf("Parsing: %s\n",instr);
      instrProc=0; 
      if (strstr(instr,"org")!=NULL)
      {
        if (it==1)
        {
          /* NOP padding */
          objCode[pc][2]='0';
          objCode[pc][3]='0';
          nops++;
        }
        pc=getNumber16(instr+3);
        it=0;
        if (pc==-1)
        {
          compFailed=1;
          compDone=1; 
          printf("Invalid ORG definition\n");
        }
        orgDef=1;
        printf("ORG defined at %d\n",pc);
        instrProc=1;
      }

      if (instrProc==0)
      {
        if (orgDef==0)
        {
          compFailed=1;
          compDone=1;
          printf("ORG not defined\n");
          instrProc=1;
        }
      }

      /* Now parse the instruction according to different cases
         The order is important because a label may contain the name of
         an instruction or a macro (any label name should be accepted).
         The priority of the cases is:
         1) Label declaration
         2) RAM data (constant)
         3) Macro
         4) Basic instruction
      */   

      if (instrProc==0)
      { 
        /* label declaration */
        if (instr[0]=='@')
        {
          instrProc=1;
          /* check if constant declaration */
          valStr=strstr(instr,"="); 
          if (valStr!=NULL)
          {
            val=getNumber16(valStr+1);
            if (val==-1)
            {
              compFailed=1;
              compDone=1;
              printf("Invalid label value\n");
            }
            labelValue[dlbl]=val;
            valStr[0]='\0'; 
          }  
          else
          {
            if (it==1)
            {
              /* NOP padding */
              objCode[pc][2]='0';
              objCode[pc][3]='0';
              pc++;
              nops++;
              it=0;
            }
            labelValue[dlbl]=pc;
          }
          strcpy(decLabelTable[dlbl],instr+1);
          printf("Label %s declared\n",decLabelTable[dlbl]);
          dlbl++;
        }
      }

      if (instrProc==0)
      {
        /* data declaration (all data are 16-bit words) */
        if (instr[0]=='#')
        {
          instrProc=1; 
          if (it==1)
          {
            /* NOP padding (not counted in efficiency stats) */
            objData[pc][2]='0';
            objData[pc][3]='0';
            pc++;
            it=0;
          }
          if (instr[1]!=34)
          {
            /* 16-bit number */
            instrArg=getNumber16(instr+1);
            if (instrArg==-1)
            { 
              compFailed=1;
              compDone=1; 
              printf("Invalid argument value\n");
            }
            else
            {
              sprintf(hexVal,"%04X", instrArg);
              objData[pc][0]=hexVal[0];
              objData[pc][1]=hexVal[1];
              objData[pc][2]=hexVal[2];
              objData[pc][3]=hexVal[3];
              ram[pc]='1';
              pc++;
            }
          }
          else
          {
            /* character string */
            n=strlen(instr)-1;
            for (j=2;j<n;j++)
            {
              val=(int)(instr[j]);
              sprintf(hexVal,"%02X",val);
              if (it==0)
              {
                objData[pc][0]=hexVal[0];
                objData[pc][1]=hexVal[1];
                ram[pc]='1';
                it=1;
              }
              else
              {
                objData[pc][2]=hexVal[0];
                objData[pc][3]=hexVal[1];
                it=0;
                pc++;
              }
            }
            if (it==1)
            {
              /* NOP padding (not counted in efficiency stats) */
              objData[pc][2]='0';
              objData[pc][3]='0';
              pc++;
              it=0;
            }
          }
        }
      }
       
      if (instrProc==0)
      { 
        /* search the macro set */
        instrFound=0;
        for (j=0;j<MAX_MSET;j++)
        {
          if (strstr(instr,macroName[j])!=NULL) 
          {
            instrFound=1;
            break;
          }
        }
        if (instrFound==1)
        {
          printf("Found macro %s\n",macroName[j]);
          n=strlen(macroCode[j])/2;
          instrCode=j;
          if (it==1)
          {
            /* NOP padding */
            objCode[pc][2]='0';
            objCode[pc][3]='0';
            pc++;
            nops++;
            it=0;
          }
          k0=pc;
          for (j=0;j<n;j++)
          {
            if (it==0)
            {
              objCode[pc][0]=macroCode[instrCode][2*j];
              objCode[pc][1]=macroCode[instrCode][2*j+1];
              rom[pc]='1';
              ubytes++;
            }
            else
            { 
              objCode[pc][2]=macroCode[instrCode][2*j];
              objCode[pc][3]=macroCode[instrCode][2*j+1];
              ubytes++;
            }
            if (it==0)
            {
              it=1;
            }
            else
            {
              it=0;
              pc++;
            }
          }
          if ((instrCode>=0) && (instrCode<=14))
          {
            /* process the argument */
            if (instr[4]=='@')
            {
              /* label used */        
              strcpy(fndLabelTable[flbl],instr+5);
              fndLabelLoc[flbl]=k0;
              flbl++;
            }
            else
            {
              instrArg=getNumber16(instr+4);
              if (instrArg==-1)
              { 
                compFailed=1;
                compDone=1; 
                printf("Invalid argument value\n");
              }
              else
              {
                sprintf(hexVal,"%04X", instrArg);
                objCode[k0][2]=hexVal[0];
                objCode[k0][3]=hexVal[1];
                objCode[k0+1][2]=hexVal[2];
                objCode[k0+1][3]=hexVal[3];
              }
            }
          }
          instrProc=1;
        }
      }          
            
      if (instrProc==0)
      { 
        /* search the basic instruction set */
        instrFound=0;
        for (j=0;j<MAX_ISET;j++)
        {
          if (strstr(instr,iset[j])!=NULL) 
          {
            instrFound=1;
            break;
          }
        }
        if (instrFound==1)        
        {
          instrCode=j;
          instrProc=1;
          /* case of instructions with 1-byte argument */
          if ((instrCode>=1) && (instrCode<=4))
          {
            instrArg=getNumber8(instr+3);
            if (instrArg==-1) 
            {
              compFailed=1;
              compDone=1; 
              printf("Invalid argument value\n");
            }
            else
            {
              if (it==1)
              {
                /* NOP padding */
                objCode[pc][2]='0';
                objCode[pc][3]='0';
                pc++;
                nops++;
                it=0;
              }
            }
          }
          sprintf(hexVal,"%02X", instrCode);
          if (it==0)
          {
            objCode[pc][0]=hexVal[0];
            objCode[pc][1]=hexVal[1];
            rom[pc]='1';
            ubytes++;
          }
          else
          { 
            objCode[pc][2]=hexVal[0];
            objCode[pc][3]=hexVal[1];
            ubytes++;
          }
          if ((instrCode>=1) && (instrCode<=4))
          {
            sprintf(hexVal,"%02X", instrArg);
            objCode[pc][2]=hexVal[0];
            objCode[pc][3]=hexVal[1];
            ubytes++;
            pc++;
          }
          else
          {
            if (it==0)
            {
              it=1;
            }
            else
            {
              it=0;
              pc++;
            }
          }
        }
      }

      if (instrProc==0)
      {
        compFailed=1;
        compDone=1;
        printf("Syntax error: %s\n",instr);
      }

    } /* while (endLine==0) */ 
    if (readLine(fsource,sLine)==0) compDone=1;
  } /* while (compDone==0) */
  
  if (it==1)
  {
    /* final NOP padding */
    objCode[pc][2]='0';
    objCode[pc][3]='0';
    pc++;
    nops++;
  }
  if (compFailed==1)
  {
    printf("Error in: %s\n",instr);
  } 
  else
  {
    /* label resolution */
    for (i=0;i<flbl;i++)
    {
      printf("Searching for label %s ...\n",fndLabelTable[i]);
      labelMatch=0;
      for (j=0;j<dlbl;j++)
      {
        if (strcmp(fndLabelTable[i],decLabelTable[j])==0)
        {
          labelMatch=1;
          break;                
        }
      }
      if (labelMatch==1)
      {
        printf("Match found: %s = %d\n",decLabelTable[j],labelValue[j]);
        sprintf(hexVal,"%04X", labelValue[j]);
        k0=fndLabelLoc[i];
        objCode[k0][2]=hexVal[0];
        objCode[k0][3]=hexVal[1];
        objCode[k0+1][2]=hexVal[2];
        objCode[k0+1][3]=hexVal[3];
      }
      else
      {
        printf("No match found\n");
        compFailed=1;
      }
    }
  }
  if (compFailed==0)
  {
    printf("Labels declared: %d\n",dlbl);
    printf("Uses of labels in instructions: %d\n",flbl);
    eff=100*((float)ubytes/(float)(ubytes+nops));
    printf("%d bytes of code, %d padding NOPs\n",ubytes,nops);
    printf(" => Efficiency = %.1f %% \n",eff);
  }

  writeFile("rom.mif",(char **)objCode,rom,2);
  writeFile("ram_h.mif",(char **)objData,ram,0);
  writeFile("ram_l.mif",(char **)objData,ram,1);

  for (i=0;i<8192;i++)
  {
    free(objCode[i]);
    free(objData[i]);
  }
  free(objCode);
  free(objData);
  fclose(fsource);
}


/* get number (8 bits) */
int getNumber8(char *s)
{
  int x=-1;

  printf("Number: %s\n",s);
  if (s[0]=='d')
  {
    x=atoi(s+1);
  }
  if ((s[0]=='x') || (s[0]=='&') || (s[0]=='$'))
  {
    x=strtol(s+1,NULL,16);
  }
  printf("Converted to: %d\n",x);
  if ((x<0) || (x>255)) x=-1;
  return(x);
}

     
/* get number (16 bits) */
int getNumber16(char *s)
{
  int x=-1;

  printf("Number: %s\n",s);
  if (s[0]=='d')
  {
    x=atoi(s+1);
  }
  if ((s[0]=='x') || (s[0]=='&') || (s[0]=='$'))
  {
    x=strtol(s+1,NULL,16);
  }
  printf("Converted to: %d\n",x);
  if ((x<0) || (x>65535)) x=-1;
  return(x);
}

/* read line */
int readLine(FILE *f,char *s)
{
  int i=0;
  char c;
  int inString=0;
  int done=0;
  int r;

  while (done==0)
  { 
    c=getc(f);
    if (c==EOF) done=1;
    if (c=='\n') done=1;
    if ((c=='!') && (inString==0))
    {
      while ((c!=EOF) && (c!='\n')) c=getc(f);
      done=1;
    }
    if (done==0)
    {
      /* check if constant string (framed by ") is being read */
      if (c==34)
      {
        if (inString==0) inString=1; else inString=0;
      }
      if ((inString==1) || ((inString==0) && (c!=32) && (c!=9)))
      {
        s[i]=c;
        i++;
      }
    }
  }
  if (c==EOF)
  {
    r=0;
  }
  else
  {
    s[i]='\0';
    r=1;  
  }
  return(r);
}

/* write object (.mif) file */
int writeFile(char *name,char **obj,char *mem,int b)
{
  FILE *fo;
  char v[5];
  int i,j,k,r;

  r=1;
  fo=fopen(name,"w");
  if (fo==NULL)
  {
    printf("Cannot create file %s\n",name);
    r=-1;
  }
  if (b==2)
  {
    fprintf(fo,"WIDTH=16;\n");
  }
  else
  {
    fprintf(fo,"WIDTH=8;\n");
  }
  fprintf(fo,"DEPTH=8192;\n\n");
  fprintf(fo,"ADDRESS_RADIX=HEX;\n");
  fprintf(fo,"DATA_RADIX=HEX;\n\n");
  fprintf(fo,"CONTENT BEGIN\n\n");

  i=0;
  while (i<8192)
  {
    k=0;
    if (mem[i]=='0')
    {
      /* see if block of 0s has to be written */
      if ((mem[i+1]=='0') && (i<8192))
      {
        j=i;
        while((mem[i]=='0') && (i<8192)) i++;
        k=1;
      }
    }
    if (k==1)
    {
      i--;
      if (b==2)
      {
        fprintf(fo,"[%04X..%04X] : 0000;\n",j,i);
      }
      else
      {
        fprintf(fo,"[%04X..%04X] : 00;\n",j,i);
      }
    }
    else
    {
      switch (b)
      {
        case 0: v[0]=obj[i][0];
                v[1]=obj[i][1];
                v[2]='\0';
                break;
        case 1: v[0]=obj[i][2];
                v[1]=obj[i][3];
                v[2]='\0';
                break;
        case 2: v[0]=obj[i][0];
                v[1]=obj[i][1];
                v[2]=obj[i][2];
                v[3]=obj[i][3];
                v[4]='\0'; 
                break;
      }
      fprintf(fo,"%04X : %s;\n",i,v);
    }
    i++;
  }
  fprintf(fo,"END;\n");
  fclose(fo);
  return(r);
}
        

      
    
   
  
       
