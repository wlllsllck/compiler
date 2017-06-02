struct A {                                              // CSubset สามารถ parse struct ได้โดยไม่มีsyntactic error
    long x, y;                                          // CSubset สามารถ parse type long ได้โดยไม่มีsyntactic error
    struct B {                                          // CSubset สามารถ parse nested struct ได้โดยไม่มีsyntactic error
        long q, r, s;                                   // CSubset สามารถ parse type long ได้โดยไม่มีsyntactic error
    } z;                
} a, b[3];                                              // CSubset สามารถ parse struct ของ array ได้โดยไม่มีsyntactic error 

void main() {                                           // CSubset สามารถ parse function ที่ไม่มีการ return ค่า (return void) ได้โดยไม่มีsyntactic error   
     long i;                                            // CSubset สามารถ parse type long ได้โดยไม่มีsyntactic error
     struct A c;                                        // CSubset สามารถ parse struct ได้โดยไม่มีsyntactic error
     struct B qq;                                       // CSubset สามารถ parse nested struct ได้โดยไม่มีsyntactic error
     c.z.r = 987654321;                                 // CSubset สามารถ parse ค่าของ nested struct ได้โดยไม่มีsyntactic error
     b[b[a.x-1].x-1].x = 7;                             // CSubset สามารถ parse ค่าของ struct ของ array ได้โดยไม่มีsyntactic error
     WriteLong(a.x); WriteLong(a.y);                    // CSubset สามารถ parse ค่าใน struct และ write ออกมาได้โดยไม่มีsyntactic error
     i = 0;                                             // CSubset สามารถ parse ตั้งค่าใน type long ได้โดยไม่มีsyntactic error                                  
     while (i < 3) {                                    // CSubset สามารถ parse while block ได้โดยไม่มีsyntactic error
        WriteLong(b[i].x); WriteLong(b[i].y);           // CSubset สามารถ parse ค่าใน struct และ write ออกมาได้โดยไม่มีsyntactic error
        i = i + 1;                                      // CSubset สามารถ parse ตั้งค่าใน type long ได้โดยไม่มีsyntactic error   
     }      
}