using System;

namespace CorlibTests.InternalCall
{
    internal class TC_System_RuntimeTypeHandle : GeneralTestCaseBase
    {
        [UnitTest]
        public void ByRefLike()
        {
            var t = typeof(int);
            Assert.False(t.IsByRefLike);
        }

        [UnitTest]
        public void ByRefLike_Span()
        {
            var t = typeof(Span<int>);
            Assert.True(t.IsByRefLike);
        }
    }
}
